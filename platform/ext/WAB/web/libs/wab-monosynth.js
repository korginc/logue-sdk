// Web Audio Blocks (WABs)
// simple semimodular monophonic synth with oscillator and effect slots
// Jari Kleimola 2019 (jari@webaudiomodules.org)
//
// audio path : (osc) -> (effect) -> amp -> actx.destination
// modulation : lfo -> lfoamount -> osc
// visuals : osc -> scope
// visuals : osc -> spect
//

var WAB = WAB || {}

WAB.MonoSynth = class WABMonoSynth
{
  constructor () {
    this.oscpitch = 64;
    this.holdon = true;
    this.lfoMaxRate = 10;
  }

  async init () {
    await this._initAudioGraph();
    this.midiHandler = new WAB.MIDIHandler();
    await this.midiHandler.init();
  }

  get oscillator () { return this.osc; }

  set oscillator (osc) {
    if (this.osc) this.osc.disconnect();
    this.lfoamount.disconnect();

    this.lfoamount.connect(osc);
    osc.connect(this.amp);
    osc.connect(scope.input);
    osc.connect(spect.input);
    this.osc = osc;

    osc.pitch = this.oscpitch;
    osc.gate = this.holdon ? 1 : 0;
  }

  set hold (onoff) { this.holdon = onoff; this.gate = onoff; }
  set gate (onoff) { this.oscillator.gate = onoff; }
  set pitch (p)    { this.oscillator.pitch = this.oscpitch = p; }

  set lfoAmount (v)   { this.lfoamount.gain.value = v; }
  set lfoRate (v)     { this.lfo.frequency.value = v * this.lfoMaxRate; }
  set masterGain (v)  { this.amp.gain.value = v; }

  onmidi (msg) {
    let status = msg[0] & 0xF0;
    if (status == 0x80 || (status == 0x90 && msg[2] == 0)) {
      if (!this.holdon) this.oscillator.gate = 0;
    }
    else if (status == 0x90) { this.pitch = msg[1]; this.gate = 1; }
  }

  // init web audio api graph
  //
  async _initAudioGraph (awpscript) {
    this.amp  = actx.createGain();
    this.lfo  = actx.createOscillator();
    this.lfoamount = actx.createGain();

    this.amp.gain.value = 0;
    this.lfo.frequency.value = 1;
    this.lfo.type = "triangle";
    this.lfoamount.gain.value = 0;
    this.lfo.connect(this.lfoamount);
    this.amp.connect(actx.destination);
    this.lfo.start();
  }
}
