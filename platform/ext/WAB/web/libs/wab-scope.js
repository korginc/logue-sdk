// Web Audio Blocks (WABs) oscilloscope widget
// Jari Kleimola 2014/2019 (jari@webaudiomodules.org)

function Oscilloscope(container, actx, frameLength)
{
	// -- visual
  if (typeof container == "string") container = document.getElementById(container);
	var dpr = window.devicePixelRatio || 1;
	this.W = container.clientWidth  - 10;
	this.H = container.clientHeight - 10;
	var c = document.createElement("canvas");
	c.width  = this.W * dpr;
	c.height = this.H * dpr;
  c.style.width  = this.W + "px";
  c.style.height = this.H + "px";
	this.ctx = c.getContext("2d");
	this.ctx.scale(dpr,dpr);
	this.canvas = c;
	container.appendChild(c);
	this.clear();

  // -- audio
	this.input = actx.createAnalyser();
	this.setFrameLength(frameLength);
	this.setLength(this.W * 2);
}

Oscilloscope.prototype = {
	setLength: function (length) {
		this.length = Math.min(this.frameLength, length);
		this.step = 1;
		if (this.length > this.W)
			this.step = this.length / this.W;
		},
	setFrameLength: function (length) {
		this.frameLength = length || 2048;
		this.input.fftSize = this.frameLength;
		this.wave = new Float32Array(this.frameLength);
		},
	clear: function () {
		this.ctx.clearRect(0, 0, this.W, this.H);
		this.ctx.beginPath();
		this.ctx.strokeStyle = "#333";
    this.ctx.strokeWidth = 1;
    let y = 0;
    let dy = this.H / 4;
    for (let i=0; i<5; i++) {
      this.ctx.moveTo(this.W, y);
      this.ctx.lineTo(0, y);
      y += dy;
    }
		this.ctx.moveTo(0, 0);
		this.ctx.lineTo(0, this.H);
		this.ctx.moveTo(this.W, 0);
		this.ctx.lineTo(this.W, this.H);
		this.ctx.stroke();
		},
	update: function () {
    let wave = new Float32Array(this.frameLength);
		this.input.getFloatTimeDomainData(wave);
    let pos = this.sync(wave);

		this.clear();
		this.ctx.strokeStyle = "#999";
		this.ctx.beginPath();

		let x = 0;
		let L = pos + this.length;
		for (let i = pos; i < L; i += this.step)
		{
			let y = this.H * (0.5 - 0.5 * wave[i|0]);
			this.ctx.lineTo(x++, y);
      if (x > this.W) break;
		}
		this.ctx.stroke();
	},
  sync: function (wave) {

    // -- find zero-crossings
    let c = [];
    let L = this.frameLength;
    for (let n=1; n<L-1; n++) {
      if (wave[n+1] * wave[n-1] <= 0 && wave[n+1] != 0) {
        c.push({ n:n, y:wave[n+1] });
        n += 5;
      }
    }
    if (!this.crossings) {
      this.crossings = c;
      return c[0] ? c[0].n : 0;
    }
    if (!c.length) return 0;

    // -- find closest match wrt previous frame
    let min = Number.MAX_SAFE_INTEGER;
    let hit = 0;
    let N = Math.min(c.length, this.crossings.length);
    for (let i=1; i<N; i++) {
      let d1 = c[i].n - c[i-1].n;
      let er = 0;
      for (let j=1; j<2; j++) {
        let d2 = this.crossings[j].n - this.crossings[j-1].n;
        er += Math.abs(d1-d2);
      }
      if (er < min) {
        min = er;
        hit = i-1;
      }
    }

    if (c[hit].y < 0) hit++;
    this.crossings = c.slice(hit);
    return c[hit] ? c[hit].n : 0;
  }

};
