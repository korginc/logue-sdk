// KORG prologue / minilogue_xd user osc/effect
// Jari Kleimola 2019 (jari@webaudiomodules.org)

var KORG = KORG || {}

// ----------------------------------------------------------------------------
// abstract superclass for logue oscillator/effect
//
KORG.LogueBlock = class LogueBlock extends WAB.Node
{
    constructor (actx, type, numInputs, numOutputs) {
        let inputChannelCount  = type === "logueosc" ? [1] : [2];
        let outputChannelCount = type === "logueosc" ? [1] : [1];
        super(actx, "logue-processor", inputChannelCount, outputChannelCount);
        this.port.onmessage = this.onmessage.bind(this);
    }

    // -- gui calls these methods in response to user interaction
    // -- actually, the current gui implementation calls setParam() directly
    //
    set param1 (value) { this.setParam(0, value); }
    set param2 (value) { this.setParam(1, value); }
    set param3 (value) { this.setParam(2, value); }
    set param4 (value) { this.setParam(3, value); }
    set param5 (value) { this.setParam(4, value); }
    set param6 (value) { this.setParam(5, value); }
    set shape (value)  { this.setParam(6, value); }
    set shiftShape (value) { this.setParam(7, value); }

    // -- gets embedded manifest
    // -- (makefile embeds manifest in oscillator js code)
    //
    async getManifest () {
        return new Promise((resolve) => {
            this.manifestResponse = resolve;
            this.postMessage("get", "manifest");
        })
    }

    // -- messages from processor appear here
    //
    onmessage (e) {
        let msg = e.data;
        if (msg.type == "manifest" && this.manifestResponse) {
            this.manifestResponse(msg.data);
            this.manifestResponse = undefined;
        }
        else super.onmessage(e);
    }
}

// ----------------------------------------------------------------------------
// oscillator
//
KORG.LogueOsc = class LogueOsc extends KORG.LogueBlock
{
    constructor (actx) {
        super(actx, "logueosc");
    }

    set gate (onoff)  { this.setParam(100, onoff); }
    set pitch (p)     { this.setParam(101, p); }
    set cutoff (f)    { this.setParam(102, f); }
    set resonance (r) { this.setParam(103, r); }

    // -- loads and instantiates the DSP part
    // -- url : oscillator code (emscripten module with embedded wasm)
    // -- needsWaves : true if the oscillator uses built-in logue waveforms -- disabled
    //
    async load (url, needsWaves) {
        await super.load(url, 0);
        // if (needsWaves) this.loadWaves();
    }

    // -- loads wavetables to simulate built-in logue waveforms
    // -- each bank is in a zip file, containing N wav files
    // -- wavs are resampled and interpolated to correct length, and merged into a single wavetable
    // -- waveforms below from https://github.com/KristofferKarlAxelEkstrand/AKWF-FREE
    //
    // async loadWaves (zips, prefix) {
    //     zips = zips || ["akwf-hvoice.zip", "akwf-clavinet.zip", "akwf-eorgan.zip", "akwf-granular.zip", "akwf-snippets.zip", "akwf-symetric.zip"]
    //     prefix = prefix || "res/wavs/";

    //     for (let i = 0; i < zips.length; i++) {
    //         let waveset = new WAB.WaveSet();
    //         await waveset.load(prefix + zips[i]);
    //         let audio = await waveset.resample(actx, 128);
    //         this.postMessage("set", 100 + i, audio.buffer);
    //     }
    // }
}

// ----------------------------------------------------------------------------
// effect (todo)
//
KORG.LogueEffect = class LogueEffect extends KORG.LogueBlock
{
    constructor (actx) {
        super(actx, "loguefx");
    }
}
