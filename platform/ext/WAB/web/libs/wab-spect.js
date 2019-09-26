// Web Audio Blocks (WABs) spectrum analyzer widget
// Jari Kleimola 2014/2019 (jari@webaudiomodules.org)
// based on http://greweb.me/2013/08/FM-audio-api/

function SpectrumAnalyzer(container, actx, minHz, maxHz, minDb, maxDb, fftSize)
{
	// -- audio
	this.sr = actx.sampleRate;
	this.input = actx.createAnalyser();
	this.input.fftSize = fftSize || 2048;
	this.input.maxDecibels = maxDb || 0;
	this.input.minDecibels = minDb || -100;
  this.input.smoothingTimeConstant = 0.5;
	this.minHz = minHz || 0;
	this.maxHz = maxHz || (actx.sampleRate / 2);
	this.spect = new Float32Array(this.input.frequencyBinCount);

	// -- visual
  if (typeof container == "string") container = document.getElementById(container);
	let dpr = window.devicePixelRatio || 1;
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
	this.ticks = true;
	this.labels = true;
	container.appendChild(c);
	this.clear();

  this.setFreqRange = function (minHz, maxHz) {
    this.minHz = minHz;
    this.maxHz = maxHz;
  }
}

SpectrumAnalyzer.prototype = {
	clear: function () {
		this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
		if (this.ticks || this.labels) this.drawOverlay();
	},
	update: function () {
		var ctx = this.ctx;
		var length = this.spect.length;
		var fftSize = this.input.fftSize;
		var minDb = this.input.minDecibels;
		var maxDb = this.input.maxDecibels;
		var W = this.W;
		var H = this.H;
		var iStart = (fftSize * this.minHz / this.sr) | 0;
		var iStop  = (fftSize * this.maxHz / this.sr) | 0;
		var range  = iStop - iStart;

		this.input.getFloatFrequencyData(this.spect);

		ctx.clearRect(0,0,W,H);
		ctx.beginPath();
		for (var i=iStart; i<=iStop; i+=2)
		{
			var x = W * (i - iStart) / range;
			var y = H * (1 - (this.spect[i] - minDb) / (maxDb - minDb));
			ctx.moveTo(x, H);
			ctx.lineTo(x, y);
		}
		ctx.strokeStyle = "#999";
		ctx.stroke();
		if (this.ticks || this.labels) this.drawOverlay();
	},
	drawOverlay: function()
	{
		var step = this.findNiceRoundStep(this.maxHz, 4);
		var prefix = step>=1000 ? "k" : "";
		var ctx = this.ctx;
		ctx.strokeStyle = "#777";
		ctx.fillStyle = "#777";
		ctx.font = "10px sans-serif";
		ctx.textAlign = "center";
		ctx.textBaseline = "top";
		for (var i=this.minHz+step; i<this.maxHz; i+=step)
		{
			var x = this.W * i/this.maxHz;
			ctx.beginPath();
			if (this.ticks)
			{
				ctx.moveTo(x, 0);
				ctx.lineTo(x, 5);
				ctx.stroke();
			}
			if (this.labels)
			{
				var text = prefix=="k" ? Math.round(i/1000) : i;
				ctx.fillText(text, x, 6);
			}
		}
	},
	findNiceRoundStep: function (delta, preferredStep) {
		var n = delta / preferredStep;
		var p = Math.floor(Math.log(n)/Math.log(10)); // powOf10(n);
		var p10 = Math.pow(10, p);
		var digit = n/p10;

		if(digit<1.5)			digit = 1;
		else if(digit<3.5)	digit = 2;
		else if(digit < 7.5)	digit = 5;
		else {
			p += 1;
			p10 = Math.pow(10, p);
			digit = 1;
		}
		return digit * p10;
	}
};