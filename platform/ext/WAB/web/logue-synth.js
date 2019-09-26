// KORG prologue / minilogue_xd user osc/fx container
// Jari Kleimola 2019 (jari@webaudiomodules.org)

var KORG = KORG || {}

var oscDefs = [
    // -- KORG
    { label:"KORG waves",   man:"KORG", type:"waves",   code:"waves.js" },

];
oscDefs.url = "oscs/";
oscDefs.defaultType = "waves"

KORG.LogueSynth = class LogueSynth extends WAB.MonoSynth
{
    async init () {
        await super.init();

        this.gui = new LogueGUI(this);
        await this.gui.init();

        // -- processor part lives in AudioWorklet's audio thread
        // -- the line below loads its script into AudioWorkletGlobalScope
        // -- instantiated in setOscType
        await actx.audioWorklet.addModule("oscs/logue-proc.js");

        let type = window.location.hash ? window.location.hash.substring(1) : oscDefs.defaultType;
        await this.setOscType(type);
        this.pitch = 64;
        this.gate  = 1;
    }

    // -- oscillator hotswap
    //
    async setOscType (type) {
        for (let i = 0; i < oscDefs.length; i++) {
            if (oscDefs[i].type == type) {

                let def = oscDefs[i];
                let url = oscDefs.url;
                if (def.man != "") url += def.man + "/"

                // -- create oscillator
                let osc = new KORG.LogueOsc(actx);
                await osc.load(url + def.code, def.waves);

                // -- setup gui knobs and insert osc into audio graph
                // -- finally, sets the defaults (unavailable in hw though)
                let manifest = await osc.getManifest();
                this.gui.reset(manifest);
                this.oscillator = osc;
                this.gui.setDefaults(manifest);

                this.type = type;
                break;
            }
        }
    }

    reload (kind) {
        if (kind == "osc") this.setOscType(this.type);
    }
}
