---
# To modify the layout, see https://jekyllrb.com/docs/themes/#overriding-theme-defaults

layout: home
---

The *logue SDK* is a software development kit and API that allows to create custom oscillators, synths, and effects for the KORG [prologue](https://www.korg.com/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd), [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1), [Nu:Tekt NTS-1 digital kit mkII](https://www.korg.com/products/synthesizers/nts_1_mk2), [Nu:Tekt NTS-3 kaoss pad kit](https://www.korg.com/products/dj/nts_3) and [drumlogue](https://www.korg.com/products/drums/drumlogue).

Singular pieces of custom content created with the SDK are commonly refered to as *units*. Each target platform can support certain unit types and not others, depending on the instrument's design and signal path.

## prologue, minilogue-xd, and NTS-1 digital kit (mkI and mkII)

These products allow four types of custom units to be loaded: oscillators, modulation effects, delay effects, and reverb effects.

### Oscillators

Custom oscillators are self contained sound generators, which are expected to provide a steady audio signal via a buffer processing callback. These are processed as part of the target platform's voice structure, meaning that articulation and filtering is already taken care of, the oscillator need only to provide a waveform according to the specified pitch information, and other available parameters.

On the [prologue](https://www.korg.com/products/synthesizers/prologue) and [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd), an oscillator unit runtime is provided for each analog voice, which signals are mixed through the analog path along with the analog voice. On the [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1) and [Nu:Tekt NTS-1 digital kit mkII](https://www.korg.com/products/dj/nts_1_mkii), a single oscillator runtime is provided, integreted with the all-digital signal path.

### Modulation Effects

Modulation effects are insert effects processed after voice articulation and filtering, and before delay and reverb effects. 

To support the [prologue](https://www.korg.com/products/synthesizers/prologue)'s dual timbre feature, the API provides two processing buffers which must be handled in the same manner. On [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd) and [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1), the second buffer can be safely ignored. 

On the [Nu:Tekt NTS-1 digital kit mkII](https://www.korg.com/products/synthesizers/nts_1_mk2), the second buffer is not present and the processing API has been uniformized with that of other effects.

### Delay and Reverb Effects

Delay and reverb effects are both send effects that are processed after the modulation effects. 

On the [prologue](https://www.korg.com/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd), and [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1), custom delay and reverb effects are loaded into the same runtime, hence when both delay and reverb effects are enabled, only one of them can be a custom effect. However, any combination of internal and custom delay and reverb effects is allowed.

On the [Nu:Tekt NTS-1 digital kit mkII](https://www.korg.com/products/synthesizers/nts_1_mk2), delay and reverb runtimes are independent from each other and thus units can be loaded without restrictions.

### Binary Compatibility

Units built for [prologue](https://www.korg.com/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd), and first generation [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1) are binary compatible accross these products.
However, it is recommended that units be optimized for each product, and thus if a product-specific unit build is available it should be preferred.

### Further Platform Details

 Refer to the following README files for further details:
 
 * [prologue platform](https://github.com/korginc/logue-sdk/tree/master/platform/prologue/README.md)
 * [minilogue-xd platform](https://github.com/korginc/logue-sdk/tree/master/platform/minilogue-xd/README.md)
 * [NTS-1 digital kit platform](https://github.com/korginc/logue-sdk/tree/master/platform/nutekt-digital/README.md)
 * [NTS-1 digital kit mkII platform](https://github.com/korginc/logue-sdk/tree/master/platform/nts-1_mkii/README.md)

## NTS-3 kaoss pad kit

The [Nu:Tekt NTS-3 kaoss pad kit](https://www.korg.com/products/dj/nts_3) provides four identical effects runtimes in which *generic effects* units can be loaded.

### Generic Effects

These effects are meant to be purpose-agnostic and the [Nu:Tekt NTS-3 kaoss pad kit](https://www.korg.com/products/dj/nts_3) allows to freely combine them using various routings.
In addition to the general purpose effect APIs, some [Nu:Tekt NTS-3 kaoss pad kit](https://www.korg.com/products/dj/nts_3)-specific APIs are provided to receive touch events, and access other platform specific features.

### Further Platform Details

 Refer to the [NTS-3 kaoss pad kit platform README](https://github.com/korginc/logue-sdk/tree/master/platform/nts-3_kaoss/README.md) for further details.

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

## Examples by KORG DIY CLUB

[KORG Examples](11_korg_examples.md)

These are examples of logue units created by KORG DIY CLUB, a group of KORG employees who love DIY and consist of software engineers and non-software engineers.
*.prlgunit*, *.mnlgxdunit*, and  *.ntkdigunit* files are simple zip files containing the binary payload for the custom oscillator or effect and a metadata file describing it.
They can be loaded onto a device matching the target platform using the [logue-cli utility](https://github.com/korginc/logue-sdk/tree/master/tools/logue-cli) or the Librarian application for that device (see product page on [KORG's official website](https://korg.com)).
