// Web Audio Blocks (WABs) AudioWorkletNode part
// Jari Kleimola 2019 (jari@webaudiomodules.org)

var WAB = WAB || {}

WAB.Node = class WABNode extends AudioWorkletNode
{
  constructor (actx, processorType, inputChannelCount, outputChannelCount) {
    let options = { processorOptions:{} };
    options.numberOfInputs  = inputChannelCount.length;
    options.numberOfOutputs = outputChannelCount.length;
    options.outputChannelCount = outputChannelCount;
    options.processorOptions.inputChannelCount = inputChannelCount;

    super(actx, processorType, options);
    this.port.onmessage = this.onmessage.bind(this);
  }

  // -- loads and instantiates the DSP part
  //
	async load (url, itype) {
    let resp = await fetch(url);
    let code = await resp.text();
    return new Promise((resolve,reject) => {
      this.resolve = resolve;
      this.reject  = reject;
		  this.postMessage("create", itype, code);
    });
	}

  // -- messages from processor appear here
  //
  onmessage (e) {
    let msg = e.data;
    if (msg.type == "response") {
      if (msg.data == "ready") this.resolve();
      else if (msg.data == "error") this.reject();
      this.resolve = this.reject = undefined;
    }
    else super.onmessage(e);
  }

  setParam (key, value) { this.postMessage("set", "param", { key:key, value:value }); }

  postMessage (verb, prop, data) {
    this.port.postMessage({ type:"msg", verb:verb, prop:prop, data:data });
  }
}
