// Web Audio Blocks (WABs) skinnable SVGKnob widget
// Jari Kleimola 2019 (jari@webaudiomodules.org)

var WAB = WAB || {}
if (!WAB.SVGKnob) {

WAB.SVGKnob = class WabSvgKnob extends HTMLElement
{
  constructor() {
    super();
    this._ = { value:0, scaledValue:0 };
    this.min = 0;
    this.max = 100;
  }

  set src (url) { setSource(url); }

  async setSource (url) {
    let res = await fetch(url);
    let svg = await res.text();
    this.innerHTML = svg;
    this.knob = this.querySelector("#knob");
    this.valueText = this.querySelector("#value");
    this.labelText = this.querySelector("#label");
    this.mouseHandler = this.onmouse.bind(this);
    this.knob.onmousedown = this.mouseHandler;
    this.querySelector("#label").textContent = this.getAttribute("label");
  }

  set value (v) {
    this._.value = v;
		let scaledValue = Math.round( this.min + (v * (this.max - this.min)) );
		if (scaledValue != this._.scaledValue) {
			this._.scaledValue = scaledValue;
			let txt = scaledValue;
			if (this._.unit) txt += " " + this._.unit;
			this.valueText.textContent = txt;
      if (this.onvalue && this.event) this.onvalue(this, v, this.event);
		}
		let angle = -150 + this._.value * 300;
		this.knob.style.transform = "rotate(" + angle + "deg)";
  }

  set label (txt) {
    let label = this.querySelector("#label");
    if (label) label.textContent = txt;
  }

  set unit (txt) {
    this._.unit = txt;
    this.value = this._.value;
  }

  reset (label) {
    this.value = 0;
    this.min = 0;
    this.max = 100;
    this.unit = "";
    this.label = label ? label : "";
  }

  updateValue (dx,dy,e) {
    var value = this._.value + dy / 150;
    if (value < 0) value = 0;
    if (value > 1) value = 1;
    value = value.toFixed(2) / 1;
    if (value != this._.value) {
			this.event = e;
      this.value = value;
			this.event = undefined;
    }
  };

  onmouse (e) {
    if (e.type == "mousedown") {
      this._.prevx = e.clientX;
      this._.prevy = e.clientY;
      window.addEventListener("mousemove", this.mouseHandler, true);
      window.addEventListener("mouseup", this.mouseHandler, true);
      document.body.className = "dragging";
    }
    else if (e.type == "mousemove") {
      var dx = this._.prevx - e.clientX;
      var dy = this._.prevy - e.clientY;
      this.updateValue(dx,dy,e);
      this._.prevx = e.clientX;
      this._.prevy = e.clientY;
    }
    else {
      window.removeEventListener("mousemove", this.mouseHandler, true);
      window.removeEventListener("mouseup", this.mouseHandler, true);
      document.body.className = "";
      e.stopPropagation();
    }
  }
}

window.customElements.define("wab-svgknob", WAB.SVGKnob);
}
