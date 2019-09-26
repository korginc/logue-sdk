// WAB WASM AudioWorkletProcessor
// Jari Kleimola 2019 (jari@webaudiomodules.org)

var WAB = WAB || {}
export default WAB;

WAB.WasmProcessor = class WAB_WasmProcessor extends AudioWorkletProcessor
{
  constructor(options) {
    super(options);
    this.audiobus = new WAB.HeapAudioBus(options);
    this.descriptor = options.processorOptions.descriptor || "";
		this.port.onmessage = this.onmessage.bind(this);
  }

  // -- messages from main thread appear here
  //
	onmessage (e) {
		let msg = e.data;
    if (!msg) return;
    switch (msg.verb) {
      case "set":
        switch (msg.prop) {
          case "param": this.wab.setParam(msg.data.key, msg.data.value); break;
        }
        break;
      case "create":
        var WABModule = { ENVIRONMENT:"WEB" };
        WABModule.onRuntimeInitialized = () => {
          this.wab = this.module.getInstance(msg.prop);
          if (this.wab.init(128, sampleRate, this.descriptor)) {
            this.module.port = this.port;
            this.audiobus.init(this.module);
            this.port.postMessage({ type:"response", data:"ready" });
          }
          else this.port.postMessage({ type:"response", data:"error" });
        }
        this.module = WABModule;
        eval(msg.data);
        break;
    }
	}

  // -- web audio api calls this method periodically
  //
  process (inputs, outputs, params) {
    let bus = this.audiobus;
    bus.push(inputs);
    this.wab.process(bus.inbufs, bus.outbufs, null);
    bus.pull(outputs);
    return true;
  }

  pushBinary (msg) {
    var buffer = new Uint8Array(msg.data);
    var len = msg.data.byteLength;
    var buf = this.module._malloc(len);
    this.module.HEAPU8.set(buffer, buf);
    this.wab.onmessage(msg.verb, msg.prop, buf, len);
    this.module._free(buf);
  }
}


// ----------------------------------------------------------------------------
// JS Float32Array <-> WASM Heap
// ----------------------------------------------------------------------------

WAB.HeapAudioBus = function (options)
{
  let ninputs  = options.numberOfInputs;
  let noutputs = options.numberOfOutputs;
  let noutChannels = options.outputChannelCount;
  let ninChannels  = options.processorOptions.inputChannelCount;
  let samplesPerBuffer = 128;
  let module = null;

  // -- preallocate WASM heap buffers
  //
  this.init = (module_) => {
    this.inbufs  = [];
    this.outbufs = [];
    module = module_;
    for (let i = 0; i < ninputs; i++)  createBuffer(module, this.inbufs,  ninChannels[i]);
    for (let i = 0; i < noutputs; i++) createBuffer(module, this.outbufs, noutChannels[i]);
  }

  function createBuffer (module, bus, nchannels) {
    for (let c = 0; c < nchannels; c++) {
      let ptr = module._malloc(samplesPerBuffer << 2);
      bus.push(ptr);
    }
  }

  // -- inputs -> WASM heap
  //
  this.push = (inputs) => {
    let j = 0;
    for (let i = 0; i < ninputs; i++) {
      let numChannels = ninChannels[i];
      for (let c = 0; c < numChannels; c++)
        module.HEAPF32.set(inputs[i][c], this.inbufs[j++] >> 2);
    }
  }

  // -- outputs <- WASM heap
  //
  this.pull = (outputs) => {
    let j = 0;
    for (let i = 0; i < noutputs; i++) {
      let numChannels = noutChannels[i];
      for (let c = 0; c < numChannels; c++) {
        let wasmout = this.outbufs[j++] >> 2;
        outputs[i][c].set(module.HEAPF32.subarray(wasmout, wasmout + samplesPerBuffer));
      }
    }
  }
}
