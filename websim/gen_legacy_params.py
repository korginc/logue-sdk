#!/usr/bin/env python3
"""Generate the websim gen-1 legacy param table from a unit's manifest.json.

Gen-1 oscillators have no unit_header, so the websim legacy bridge needs slider
metadata for the custom params. Rather than hand-maintain a WEBSIM_LEGACY_PARAM_LIST
macro in each wasm.cc (which must mirror manifest.json by hand), this emits that
macro straight from manifest.json so any gen-1 unit works without editing C.
See WEBSIM_FOLLOWUP_PLAN.md §C.2.

  gen_legacy_params.py <manifest.json> <out.h>

manifest header.params entries are [name, min, max, typestr]; typestr maps to the
legacy bridge's LP_* enum.
"""
import json
import sys

TYPE_MAP = {"": "LP_NONE", "%": "LP_PERCENT", "%%": "LP_PERCENT"}


def main(argv):
    if len(argv) != 3:
        sys.stderr.write("usage: gen_legacy_params.py <manifest.json> <out.h>\n")
        return 2
    manifest_path, out_path = argv[1], argv[2]
    with open(manifest_path) as f:
        manifest = json.load(f)
    params = manifest.get("header", {}).get("params", []) or []

    rows = []
    for p in params:
        name = str(p[0])
        mn, mx = int(p[1]), int(p[2])
        typ = str(p[3]) if len(p) > 3 else ""
        lp = TYPE_MAP.get(typ, "LP_NONE")
        rows.append('  X("%s", %d, %d, %s)' % (name, mn, mx, lp))

    out = []
    out.append("// Generated from %s by websim/gen_legacy_params.py — do not edit." % manifest_path)
    out.append("#ifndef WEBSIM_LEGACY_PARAMS_H_")
    out.append("#define WEBSIM_LEGACY_PARAMS_H_")
    if rows:
        out.append("#define WEBSIM_LEGACY_PARAM_LIST(X) \\")
        out.append(" \\\n".join(rows))
    else:
        # No custom params; the bridge still appends SHAPE / SHIFT-SHAPE.
        out.append("#define WEBSIM_LEGACY_PARAM_LIST(X)")
    out.append("#endif  // WEBSIM_LEGACY_PARAMS_H_")

    with open(out_path, "w") as f:
        f.write("\n".join(out) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
