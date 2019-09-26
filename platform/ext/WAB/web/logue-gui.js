// KORG prologue / minilogue_xd user osc/fx gui
// Jari Kleimola 2019 (jari@webaudiomodules.org)

var LogueGUI = function (synth)
{
    var shapeValue1 = 0;
    var shapeValue2 = 0;
    var lfoValue1 = 0;
    var lfoValue2 = 0.32;

    // ==========================================================================
    // init
    // ==========================================================================

    // -- init widgets
    //
    this.init = async () => {
        // -- oscillator combo
        let combo = document.getElementById("osc");
        let prevman = oscDefs[0].man;
        oscDefs.forEach((o) => {
            if (prevman != o.man) {
                prevman = o.man;
                combo.appendChild(new Option("---------------"));
                combo.lastElementChild.disabled = true;
            }
            combo.appendChild(new Option(o.label, o.type));
        });

        // -- midi input ports
        combo = document.getElementById("midiIn");
        synth.midiHandler.inputs.forEach((input) => {
            let option = new Option(input.name);
            option.port = input;
            combo.appendChild(option);
        });
        combo.onchange = e => {
            let inport = e.target.options[e.target.selectedIndex].port;
            inport.onmidimessage = (e) => { synth.onmidi(e.data); }
        }
        if (combo.options.length > 0)
            combo.onchange({ target:combo });

        // -- shape and param knobs
        let knobs = document.querySelectorAll("wab-svgknob");
        for (let i=0; i<knobs.length; i++) {
            let knob = knobs[i];
            await knob.setSource("res/logue-knob.svg");
            if (0 < i && i < 7) knob.id = "macro-" + i;
            knob.onvalue = onknob;
        }

        // -- hold and spectrum range toggles
        let toggles = document.querySelectorAll(".toggle");
        Array.from(toggles).forEach((toggle) => {
            toggle.onclick = (toggle.id == "hold") ? onhold : onrange;
        });
        holdon = true;

        // -- shift key
        document.body.onkeydown = (e) => {
            if (e.key == "Shift") onshift(true);
        }
        document.body.onkeyup = (e) => {
            if (e.key == "Shift") onshift(false);
        }

        // -- plots
        scope = new Oscilloscope("scope", actx, 4096);
        spect = new SpectrumAnalyzer("spect", actx, 0,5000, -100,-6, 32768 >> 1);

        // -- midi keyboard
        let midikeys = new QwertyHancock({
            container: document.querySelector("#keys"), height: 70,
            octaves: 5, startNote: 'C2', oct:4,
            whiteNotesColour: 'white', blackNotesColour: 'black', activeColour:'orange'
        });
        midikeys.keyDown = (note, name) => synth.onmidi([0x90, note, 100]);
        midikeys.keyUp   = (note, name) => synth.onmidi([0x80, note, 100]);

        // -- overlay
        let overlay = document.querySelector("#overlay");
        overlay.onclick = () => {
            actx.resume();
            synth.masterGain = 0.2;
            overlay.classList.add("fadeout");
            setTimeout(() => { overlay.style.display = "none"; }, 1000);
        }

        setTimeout(() => { plot(); }, 100);
    }


    // ==========================================================================
    // implementation
    // ==========================================================================

    // -- called when oscillator type is changed
    //
    this.reset = (manifest) => {
        let knobs = document.querySelectorAll("wab-svgknob");
        for (let i=0; i<knobs.length; i++) {
            let label = "PARAM " + i;
            if (i == 0) label = "SHAPE";
            else if (i == 7) label = "LFO";
            knobs[i].reset(label);
        }

        if (manifest) {
            for (let i=0; i<manifest.header.num_param; i++) {
                let param = manifest.header.params[i];
                let knob = knobs[i+1];
                knob.label = param[0];
                knob.min = param[1];
                knob.max = param[2];
                knob.unit = param[3];
            }
        }

        shapeValue1 = shapeValue2 = 0;
        lfoValue1 = 0;
        lfoValue2 = 0.32;
    }

    // -- defaults are unavailable in hw unit, but useful for development
    //
    this.setDefaults = (manifest) => {
        if (manifest && manifest.header.defaults) {
            let knobs = document.querySelectorAll("wab-svgknob");
            let knobmap = [1,2,3,4,5,6,0,0,7,7];
            for (let i=0; i<manifest.header.defaults.length; i++) {
                let def = manifest.header.defaults[i];
                let scale = (i == 6 || i == 7) ? 10 : 1;
                synth.oscillator.setParam(i, def * scale);
                if (i != 7 && i != 9) {
                    let knob = knobs[knobmap[i]];
                    knob.value = (def - knob.min) / (knob.max - knob.min);
                }
                def /= 100;
                switch (i) {
                case 6: shapeValue1 = def; break;
                case 7: shapeValue2 = def; break;
                case 8: lfoValue1 = def; break;
                case 9: lfoValue2 = def; break;
                }
            }
        }
    }

    // -- constantly updates waveform and spectrum plots
    //
    function plot () {
        scope.update();
        spect.update();
        requestAnimationFrame(plot);
    }


    // ==========================================================================
    // interaction handlers
    // ==========================================================================

    // -- called when user tweaks the shape/param/lfo knobs
    //
    function onknob (knob, value, e) {
        if (knob.id == "lfo") {
            if (e.shiftKey) {
                lfoValue2 = value;
                synth.lfoRate = value * value;
            }
            else synth.lfoAmount = lfoValue1 = value;
            return;
        }

        let paramID;
        if (knob.id == "shape") {
            if (e.shiftKey) { shapeValue2 = value; paramID = 7; }
            else { shapeValue1 = value; paramID = 6; }
            value *= 10;
        }
        else paramID = knob.id[6] - 1;
        let v = knob.min + value * (knob.max - knob.min);
        synth.oscillator.setParam(paramID, Math.round(v));
    }

    // -- called when user changes spectrum plot freq range
    //
    function onrange (e) {
        let toggles = document.querySelectorAll(".toggle");
        Array.from(toggles).forEach((toggle) => { if (toggle.id != "hold") toggle.classList.remove("active"); });
        e.target.classList.add("active");
        let upperRange = e.target.innerText * 1000;
        if (upperRange == 20000) upperRange = actx.sampleRate / 2;
        spect.setFreqRange(0, upperRange);
    }

    // -- called when user toggles HOLD
    //
    function onhold (e) {
        e.target.classList.toggle("active");
        holdon = !holdon;
        synth.hold = holdon;
    }

    // -- called when user presses/releases the shift key
    //
    function onshift (onoff) {
        let shapeKnob = document.getElementById("shape");
        shapeKnob.value = onoff ? shapeValue2 : shapeValue1;
        let lfoKnob = document.getElementById("lfo");
        lfoKnob.value = onoff ? lfoValue2 : lfoValue1;
    }
}
