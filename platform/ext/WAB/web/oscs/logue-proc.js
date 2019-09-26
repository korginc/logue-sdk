// KORG prologue / minilogue_xd user oscillator/effect
// interfaces both osc and effect instances
// Jari Kleimola 2019 (jari@webaudiomodules.org)

import WAB from "../libs/wab-processor.js"
var KORG = KORG || {}

KORG.LogueProcessor = class LogueProcessor extends WAB.WasmProcessor
{
    constructor(options) {
        super(options);
        this.port.onmessage = this.onmessage.bind(this);
    }

    onmessage (e) {
        // -- set waveset data
        let msg = e.data;
        let iprop = parseInt(msg.prop);
        if (msg.verb == "set" && iprop >= 100)
            this.pushBinary(msg);

        // -- hack to get embedded manifest
        else if (msg.verb == "get" && msg.prop == "manifest")
            this.port.postMessage({ type:"manifest", data:this.module.manifest });

        // -- other messages are routed automatically
        else super.onmessage(e);
    }
}

registerProcessor("logue-processor", KORG.LogueProcessor);
