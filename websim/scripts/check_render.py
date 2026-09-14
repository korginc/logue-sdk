#!/usr/bin/env python3
"""Headless audio-correctness check for websim offline renders (WEBSIM.md §B).

Reads a 32-bit float WAV produced by `make render` and reports basic health
(RMS, peak, NaN/Inf) plus, for oscillators, the estimated fundamental frequency.
Pure stdlib — no numpy/scipy — so it runs anywhere CI does.

Usage:
  check_render.py FILE.wav                          # health report only
  check_render.py FILE.wav --expect-hz 440 --tol-cents 50   # osc pitch gate
  check_render.py FILE.wav --min-rms 1e-4           # fail if too quiet

Exit code is non-zero if any requested check fails.
"""
import argparse
import math
import struct
import sys


def read_wav_f32(path):
    """Return (channels, sample_rate, [float samples interleaved])."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError("not a RIFF/WAVE file")
    pos = 12
    fmt = None
    samples = None
    while pos + 8 <= len(data):
        cid = data[pos:pos + 4]
        (csize,) = struct.unpack_from("<I", data, pos + 4)
        body = data[pos + 8:pos + 8 + csize]
        if cid == b"fmt ":
            tag, ch, sr, _br, _ba, bits = struct.unpack_from("<HHIIHH", body, 0)
            fmt = (tag, ch, sr, bits)
        elif cid == b"data":
            n = len(body) // 4
            samples = list(struct.unpack_from("<%df" % n, body, 0))
        pos += 8 + csize + (csize & 1)
    if fmt is None or samples is None:
        raise ValueError("missing fmt/data chunk")
    tag, ch, sr, bits = fmt
    if tag != 3 or bits != 32:
        raise ValueError("expected 32-bit IEEE float (tag 3), got tag %d/%d-bit" % (tag, bits))
    return ch, sr, samples


def mono(samples, channels):
    if channels == 1:
        return samples
    return [sum(samples[i:i + channels]) / channels
            for i in range(0, len(samples) - channels + 1, channels)]


def goertzel_power(x, sr, f):
    """Normalised power of signal x at frequency f (steady second half)."""
    x = x[len(x) // 2:]
    n = len(x)
    if n == 0 or f <= 0:
        return 0.0
    w = 2.0 * math.pi * f / sr
    c = 2.0 * math.cos(w)
    s1 = s2 = 0.0
    for v in x:
        s0 = v + c * s1 - s2
        s2, s1 = s1, s0
    return (s1 * s1 + s2 * s2 - c * s1 * s2) / (n * n)


def estimate_f0(x, sr, fmin=40.0, fmax=2000.0):
    """Autocorrelation fundamental estimate over the steadier second half."""
    x = x[len(x) // 2:]
    n = len(x)
    if n < 64:
        return 0.0
    mean = sum(x) / n
    x = [v - mean for v in x]
    lo = max(1, int(sr / fmax))
    hi = min(n - 1, int(sr / fmin))
    best_lag, best = 0, 0.0
    for lag in range(lo, hi + 1):
        s = 0.0
        for i in range(n - lag):
            s += x[i] * x[i + lag]
        if s > best:
            best, best_lag = s, lag
    return sr / best_lag if best_lag else 0.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("wav")
    ap.add_argument("--expect-hz", type=float, default=None)
    ap.add_argument("--tol-cents", type=float, default=50.0)
    ap.add_argument("--min-rms", type=float, default=None)
    ap.add_argument("--peaks", default=None,
                    help="comma-separated Hz that must all be present at comparable "
                         "energy (poly chord check)")
    ap.add_argument("--peak-ratio", type=float, default=0.1,
                    help="min(power)/max(power) across --peaks required to pass")
    args = ap.parse_args()

    ch, sr, samples = read_wav_f32(args.wav)
    nan = sum(1 for v in samples if v != v or v in (float("inf"), float("-inf")))
    finite = [v for v in samples if v == v and v not in (float("inf"), float("-inf"))]
    peak = max((abs(v) for v in finite), default=0.0)
    rms = math.sqrt(sum(v * v for v in finite) / len(finite)) if finite else 0.0

    print("file       : %s" % args.wav)
    print("format     : %d ch, %d Hz, %d frames" % (ch, sr, len(samples) // ch))
    print("peak       : %.6f" % peak)
    print("rms        : %.6f" % rms)
    print("nan/inf    : %d" % nan)

    ok = True
    if nan:
        print("FAIL: %d non-finite samples" % nan); ok = False
    if args.min_rms is not None and rms < args.min_rms:
        print("FAIL: rms %.3g < min %.3g" % (rms, args.min_rms)); ok = False
    if args.expect_hz is not None:
        f0 = estimate_f0(mono(samples, ch), sr)
        cents = 1200.0 * math.log2(f0 / args.expect_hz) if f0 > 0 else float("inf")
        print("f0 est     : %.2f Hz (expect %.2f, %.1f cents)" % (f0, args.expect_hz, cents))
        if abs(cents) > args.tol_cents:
            print("FAIL: pitch off by %.1f cents (tol %.1f)" % (cents, args.tol_cents)); ok = False
    if args.peaks:
        m = mono(samples, ch)
        freqs = [float(x) for x in args.peaks.split(",")]
        powers = [goertzel_power(m, sr, f) for f in freqs]
        pmax = max(powers) if powers else 0.0
        for f, p in zip(freqs, powers):
            rel = (p / pmax) if pmax > 0 else 0.0
            print("peak %.1f Hz : power %.3e (%.2f of max)" % (f, p, rel))
        worst = min((p / pmax) for p in powers) if pmax > 0 else 0.0
        if worst < args.peak_ratio:
            print("FAIL: a requested pitch is weak/absent (%.2f < %.2f)" % (worst, args.peak_ratio))
            ok = False

    print("RESULT     : %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
