---
# To modify the layout, see https://jekyllrb.com/docs/themes/#overriding-theme-defaults

layout: home
---

The *logue SDK* is a software development kit and API that allows to create custom oscillators, synths, and effects for the KORG [prologue](https://www.korg.com/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd), [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1) and [drumlogue](https://www.korg.com/products/drums/drumlogue).

Singular pieces of custom content created with the SDK are commonly refered to as *units*. Each target platform can support certain unit types and not others, depending on the instrument's design and signal path.

## prologue, minilogue-xd, and NTS-1

Four types of custom units can be created for these platforms: oscillators, modulation effects, delay effects, and reverb effects. 

These platforms' APIs are essentially the same and are binary compatible. However, in order to compensate for performance differences, custom units should be optimized and built for each platform separately.

### Oscillators

Custom oscillators are self contained sound generators, which are expected to provide a steady audio signal via a buffer processing callback. These are processed as part of the target platform's voice structure, meaning that articulation and filtering is already taken care of, the oscillator need only to provide a waveform according to the specified pitch information, and other available parameters.

### Modulation Effects

Modulation effects are insert effects processed after voice articulation and filtering, and before delay and reverb effects. Note that in order to support the [prologue](https://www.korg.com/products/synthesizers/prologue)'s dual timbre feature, the API provides two processing buffers which must be handled in the same manner. On [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd) and [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1), the second buffer can be safely ignored.

### Delay and Reverb Effects

Delay and reverb effects are both send effects that are processed after the modulation effects. On the [prologue](https://www.korg.com/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd), and [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1), custom delay and reverb effects are loaded into the same runtime, hence when both delay and reverb effects are enabled, only one of them can be a custom effect. However, any combination of internal and custom delay and reverb effects is allowed.

### Further Platform Details

 Refer to the following README files for further details:
 
 * [prologue platform](https://github.com/korginc/logue-sdk/tree/master/platform/prologue/README.md)
 * [minilogue-xd platform](https://github.com/korginc/logue-sdk/tree/master/platform/minilogue-xd/README.md)
 * [NTS-1 platform](https://github.com/korginc/logue-sdk/tree/master/platform/nutekt-digital/README.md)

## drumlogue

Four types of custom units can be created for this platform: synths, delay effects, reverb effects, and master effects.

The [drumlogue](https://www.korg.com/products/drums/drumlogue) custom unit API is not compatible with previously discussed target platforms, however there are similarities in the core API structure which should make porting units from one platform to the other relatively straightforward.

### Synths

Custom synths are self contained sound generators that are also responsible for voice articulation and filtering (if applicable). These are processed as an individual synth part within the [drumlogue](https://www.korg.com/products/drums/drumlogue)'s multi engine section.

### Delay Effects

Delay effects are executed in a dedicated runtime environment, and process a dedicated send bus. The output can be mixed back before or after the master effect and can also be routed to the reverb effect.

### Reverb Effects

Reverb effects are executed in a dedicated runtime environment, and process a dedicated send bus. The output can be mixed back before or after the master effect.

### Master Effects

Master effects are inline effects that can be bypassed on a per-part basis. The API also allows using the optional sidechain send bus.

### Further Platform Details

 Refer to the [drumlogue platform README](https://github.com/korginc/logue-sdk/tree/master/platform/drumlogue/README.md) for further details.

