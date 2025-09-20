Ice Moon Prison

## Resonator: a Karplus-Strong synth unit for Korg drumlogue

Korg’s drumlogue hybrid drum machine (2021 2022) continues the tradition of the logue series of hardware (prologue, minilogue xd, NTS-1) in allowing end users to develop, install and use custom coded oscillators and effects. This unit, Resonator, is an extended Karplus-Strong physical model synth which produces sounds reminiscent of plucked strings, struck metallic bars, and tuned metal drums.

## About resonators
The Karplus-Strong model for a plucked string consists of a sample (the excitation) played through a delay line. The output of the delay line is returned back to its input, with slightly less than unity amplification, resulting in a feedback loop which reinforces frequencies with a period that is a multiple of the delay, and attenuates other frequencies (the resonator). With a burst of white noise as the excitation, such a model tends to produce a decaying metallic pluck reminiscent of a steel guitar or a harpsichord.


## Block diagram of one voice of the Resonator synth unit
Extensions are commonly applied to the Karplus-Strong model to produce sounds more similar to real-world string intstruments:
![alt text](Extended-Karplus-Strong-block-diagram-1024x502.png)
* A filter is applied to the excitation signal before it is fed to the resonator feedback loop. An excitation filter changes the tone of the resonating sound, softening or harshening it, or permits pre-seeding the resonator with a preferred set of frequencies.
* The feedback signal is added to itself, delayed by one sample. This results in a very rudimentary low-pass filter and causes higher frequencies in the resonator to decay more quickly than lower frequencies. This high-frequency damping is common in real-world instruments.
* The feedback signal is passed through one or more allpass filters. This causes higher frequencies to be delayed slightly more than lower frequencies, and disrupts the normally exact harmonic nature of the feedback’s comb filter. This dispersion effect gives the resonator an inharmonic component which can be subtle or strong depending on the properties and quantities of the allpass filters.

## About this unit
Resonator uses an extended Karplus-Strong modal to create resonant sounds from drumlogue samples. It has these features as of version 1.1:

* Excitation from any sample in the drumlogue, including built-in CH, OH, RS, CP and MISC banks, as well as the two user banks USER and EXT.
* Sample trimming at start and end.
* Excitation filtering (first-order low- and high-pass; second-order low-, band- and high-pass with resonance).
* Velocity/accent sensitivity (controls excitation level and filter).
* Feedback and high-frequency damping during note-on (decay) and note-off (release) phases. Feedback can be negative or positive.
* Variable string dispersion.
* Keyboard tracking (controls feedback).
* Polyphony of up to eight notes, which allows notes to decay fully with less voice-stealing. (User synth polyphony can only be fully realized on the drumlogue by sending it MIDI input; the built-in sequencer can record and play only one note to the Multi-Engine at a time.)
* Pitch bend (only over MIDI).

## Downloading and installing
1. Download the unit from GitHub and expand the zip file.
2. Power up your drumlogue in USB storage mode by holding down the Rec button.
3. Connect the drumlogue to your computer via USB and copy the resonator.drmlgunit file to the /Units/Synths folder of the drumlogue volume.
4. Unmount or eject the drumlogue volume.
5. Press the Play button on the drumlogue to return to performance mode. The unit can be selected by pressing Part + User and turning the left parameter knob.

## Parameters
Resonator has 18 parameters, all of which can be motion sequenced. Particularly interesting parameters to motion sequence are Note (Page 1); Sample bank and number (Page 2), Excitation level, Cutoff, and Resonance (Page 3), and Decay and Release Feedback and Damping (Pages 4-5).

### Page 1: Pitch and velocity
These parameters apply to MIDI Note-On events, which have a note number and a velocity. When using the drumlogue’s front panel, the MULTI USER button acts as a Gate-On event, with the note number controlled by the Note parameter and the velocity corresponding to the step’s ACCENT level.

`Note (C-1 .. G9)` controls the played note for gate-on events triggered by the MULTI USER button. It is ignored for notes received over MIDI. The note controls the resonant frequency of the resonator through the resonator’s internal delay line: higher notes have a shorter delay line and a higher resonant frequency.

`Bend (0 .. 12)` is how many semitones that MIDI pitch bend will change the current note. Live MIDI playback only! The drumlogue’s internal sequencer cannot record or play back pitch bend.

`Velocity → Level (-100 .. 100)` dictates how note-on velocity and gate accent affect the excitation level (Page 3). 0 means velocity has no effect. 100 means velocity changes the excitation level by -6 to +6 dB for velocity 0 to 127. Negative values swap the sign of the decibel change.

`Velocity → Filter (-100 .. 100)` dictates how note-on velocity and gate accent affect the excitation filter (Page 3). 0 means velocity has no effect. 100 means velocity multiplies the cutoff frequency by up to a factor of between 0.5 and 2 (for a low-pass excitation filter), divides the cutoff frequency by up to a factor of between 0.2 and 2 (for a high-pass excitation filter), or multiplies the resonance by up to a factor of between 0.25 and 4 (for a bandpass excitation filter). Negative values swap multiplication and division.

### Page 2: Excitation sample
Each time a note-on event occurs, Resonator plays the selected sample. This sample is played as-is to the output, and also as input to the resonator’s internal delay line. In a resonator, the Note parameter does not affect the excitation sample pitch, only the resonant pitch of the delay line.

`Sample bank (CH, OH, RS, CP, MISC, USER, EXP)` selects which sample bank contains the sample to be played. User-transferred samples will appear in USER or EXP banks.

`Sample number (1 .. 128)` selects a sample from the chosen bank. All built-in banks have fewer than 128 samples; if you choose a sample that is out of range, no sample is played.

`Start (0.0% .. 100.0%)` trims from the beginning of the sample. Because most samples in the drumlogue have an abrupt onset followed by a decay, increasing Start has the effect of softening the excitation.

`End (0.0% .. 100.0%)` stops playing the sample earlier (100% plays to the end of the sample). The sample is cut off abruptly; there is no fade out. Only this End parameter dictates when the excitation sample stops playing; in particular, a MIDI note-off event won’t stop the sample (but it can change the resonator’s properties; see Page 5).

### Page 3: Excitation level and filter
The sample selected on Page 2 is adjusted through an amplification/attenuation stage and a biquad filter before it is fed into the resonator. The tonal quality of the excitation affects the tone of the sound in the resonant delay loop.

`Level (-20dB .. +6db)` adjusts the volume of the excitation sample. This should be decreased when playing polyphonic MIDI input to prevent clipping. This raw value is affected by Velocity → Level (Page 1).

`Filter type (None, LP12, HP12, BP6, LP6, HP6)` optionally filters the excitation sample through a digital filter (Low-pass, high-pass or bandpass; 6 dB/octave or 12 db/octave slope). A low-pass filter produces a less harsh pluck with more body; a high-pass filter makes for a more glassy excitation; a bandpass filter produces a more precise pitched pluck, especiallly at high resonance.

`Filter cutoff (0.1 kHz .. 20.0 kHz)` selects the cutoff frequency for the selected filter (except None). This raw value is affected by Velocity → Filter (Page 1) for low-pass and high-pass filters.

`Filter resonance (Q) (0.1 .. 20.0)` selects the resonance for 12-dB-per-octave filters and for the bandpass filter (it is ignored for LP6 and HP6). A higher resonance causes frequencies near the cutoff frequency to be amplified (and frequencies far from the cutoff to be attenuated). A resonance of about 0.7 gives a flat response curve, and lower values of Q de-emphasize the cutoff frequency. This raw value is affected by Velocity → Filter for the bandpass filter.

### Page 4: Resonator (decay phase)
The resonator itself is a delay line whose output feeds back into its input. The fed-back signal undergoes three alterations on the return path: high-frequency damping, dispersion, and attenuation.

`Decay feedback (-100.0% .. +100.0%)` controls how long the resonator sustains its sound. It controls how much of the delay line output is fed back into the input while the note is being held. At 0%, the delay line is completely attenuated and only the excitation sample can be heard. At 100%, the signal never attenuates and the sound continues forever (or until voice stealing ends the note). Values just below 100% give a gently decaying plucked string sound. Negative feedback values shift the resonant frequencies by -f/2. You will hear this as a sound an octave lower with only odd harmonics.

`Decay damping (0% .. 100%)` controls the amount of high-frequency damping in the resonator. At 0%, all frequencies decay at the same rate dictated by Decay feedback. At higher values, higher frequencies are gradually filtered out as the note decays, leaving only the fundamental frequency.

`Dispersion (0 .. 4)` introduces an inharmonic element to the resonator, by altering the length of the delay line differently for higher frequencies. Tonally, more dispersion makes the sound less like a string and more like a metal block or sheet. A value of 0 disables dispersion; higher values introduce more atonal components. Dispersion applies equally during decay (note-on) and release (note-off) stages.

`Key tracking (0 .. 100)` compensates for the natural tendency of higher notes to decay faster, by automatically altering the feedback based on the played note. A value of 0 results in a naive resonator where a note an octave higher decays at twice the rate. A value of 100 makes all notes decay at the same rate irrespective of pitch.

### Page 5: Resonator (release phase)
After the note is released, the resonator continues to sound, but the parameters on this page allow you to simulate damping (as pianists, percussionists, and guitarists understand it).

`Release feedback (-100.0% .. +100.0%)` works just as Decay feedback (Page 4). Set this value to the same as the decay feedback if you want to ignore the note-off event and let the note ring out. It is possible for the release feedback to be negative while decay feedback is positive, and vice versa (you will hear the note jump an octave).

`Release damping (0% .. 100%)` works just as Decay damping (Page 4). Set this value to a higher value than the decay damping if you want to emulate real-world musical instruments.

## Building from source
The software development kit contains the tools and code necessary to build unit files. Starting with the drumlogue in version 1.1 of the SDK, development is through a docker image runnable on most platforms.

You don’t need to compile this unit unless you want to modify it or extend it. If you just want to use the unit as-is, download it here.

The code for this user unit is released under the 3-clause BSD licence. You may modify and release derived works with the requirement that you credit the author (Deborah Pickett).

1. Download or clone the source code from GitHub.
2. Set up the docker image according to Korg’s instructions.
3. Run the command: ./run_cmd.sh build drumlogue/resonator
