## About this project

This repository is my attempt to port the [RipplerX](https://github.com/tiagolr/ripplerx/) phisical modelling synth into Korg Drumlogue ecology.
The version I'm working in this moment is freezed at 1.5.0. At this stage I removed the Juce dependencies and added the ARM Neon intrinsics for all the files (with some refactoring to allegedly speed up processing and simplifing calculations). Note that original code uses *double* type, which is not present in ARMv7, so there will a loss of precision that - I hope! - will not degrade too much the sound.
Before trying to build I have to review everything (again) and continue to reshape the main files, i.e. Voice.cpp/h (which is the modelled resonator), the ripplerx.h (which is the class that will be the interface berween physical commands of drumlogue and the program itself) and unit.cc (which the interface with the platform).
Then there will be the build, loading to a real drumlogue, troubleshooting and playtesting.

many thanks to [Ice Moon Prison](https://github.com/futzle/logue-sdk/) for the inspiration on this.


## The original RipplerX readMe
RipplerX is a physically modeled synth, capable of sounds similar to AAS Chromaphone and Ableton Collision.
# Features
- Dual resonators with serial and parallel coupling.
- 9 Models of acoustic resonators: String, Beam, Squared, Membrane, Drumhead, Plate, Marimba, - Open tube, and Closed tube.
- Inharmonicity, Tone, Ratio, and Material sliders to shape the timbre.
- Noise and mallet generators.
- Up to 64 partials per resonator.

