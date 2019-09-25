// Web Audio Blocks (WABs) util classes
// Jari Kleimola 2019 (jari@webaudiomodules.org)

var WAB = WAB || {}

// ----------------------------------------------------------------------------
// web MIDI
//
WAB.MIDIHandler = function ()
{
  let midiAccess = {};

  this.init = async () => {
    return new Promise((resolve, reject) => {
      if (navigator.requestMIDIAccess) {
        navigator.requestMIDIAccess().then((access) => {
          midiAccess = access;
          resolve(access);
        }, () => { reject(); });
      }
      else reject();
    });
  }

  Object.defineProperties(this, {
    inputs: { get: () => { return midiAccess.inputs; }}
  })
}

// ----------------------------------------------------------------------------
// wavetable collection
//
WAB.WaveSet = function ()
{
  var zip;
  this.buffer = null;

  this.load = async (url) => {
    return new Promise(async (resolve) => {
      let resp = await fetch(url);
      let data = await resp.arrayBuffer();
      let jszip = new JSZip();
      jszip.loadAsync(data).then(async (z) => {
        zip = z;
        resolve();
      });
    });
  }

  this.resample = async (actx, length) => {
    if (!zip) return null;
    let wavs = [];
    let keys = Object.keys(zip.files).sort();
    for (let i = 0; i < keys.length; i++) {
      if (keys[i].indexOf("/") >= 0) continue;
      let wavetable = await decode(zip.files[keys[i]], length);
      wavs.push(wavetable);
    }

    this.buffer = mergeWaveset(wavs);
    return this.buffer;
  }

  async function decode (file, length) {
    return new Promise((resolve) => {
      file.async("arraybuffer").then(function (abuf) {
        actx.decodeAudioData(abuf, (audio) => {
          let wavetable = resizeBuffer(audio, length);
          resolve(wavetable);
        });
      });
    })
  }

  // just linear interpolation for now
  // appends first sample to the end
  // therefore the returned buffersize is length+1
  //
  function resizeBuffer (abuf, length) {
    let src = new Float32Array(abuf.length + 1);
    let dst = new Float32Array(length+1);
    let phase = 0;
    let phinc = src.length / length;
    src.set(abuf.getChannelData(0));
    src[src.length-1] = src[0];
    for (let n=0; n<length; n++) {
      let n1 = phase|0;
      let f  = phase - n1;
      let x1 = src[n1];
      let x2 = src[n1 + 1];
      dst[n] = (1-f) * x1 + f * x2;
      phase += phinc;
    }
    dst[length] = dst[0];

    return dst;
  }

  function mergeWaveset (waves) {
    let length = 0;
    waves.forEach((w) => { length += w.length; });
    let waveset = new Float32Array(length);
    let n = 0;
    waves.forEach((w) => {
      waveset.set(w, n);
      n += w.length;
    });
    return waveset;
  }

}
