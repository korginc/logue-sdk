# logue-sdk 

[![Discord](https://img.shields.io/discord/1037767234803740694?logo=discord&label=Discord)](https://discord.com/invite/pqW5VpdQkZ)

[日本語](./README_ja.md)

This repository contains all the files and tools needed to build custom oscillators and effects for the [prologue](https://www.korg.com/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd), [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1), [Nu:Tekt NTS-1 digital kit mkII](https://www.korg.com/products/synthesizers/nts_1_mk2), [microKORG2](https://www.korg.com/products/synthesizers/microkorg2) synthesizers, the [Nu:Tekt NTS-3 kaoss pad kit](https://www.korg.com/products/dj/nts_3), and [drumlogue](https://www.korg.com/products/drums/drumlogue) drum machine.

## Existing logue SDK Units

To download ready to use oscillators and effects, refer to the [Unit Index](https://korginc.github.io/logue-sdk/unit-index/) and follow instructions on the developer's website.

There is an easier to navigate unit index page called [logue-SDK-filter](https://logue-sdk.vercel.app/).

## Platforms and Compatibility

| Product                        | Latest SDK Version | Minimum Firmware Version | CPU Arch.     | Unit Format                                                 |
|--------------------------------|--------------------|--------------------------|---------------|-------------------------------------------------------------|
| prologue                       | v1.1.0             | >= v2.00                 | ARM Cortex-M4 | Custom 32-bit LSB executable, ARM, EABI5 v1 (SYSV), static  |
| minilogue-xd                   | v1.1.0             | >= v2.00                 | ARM Cortex-M4 | Custom 32-bit LSB executable, ARM, EABI5 v1 (SYSV), static  |
| Nu:Tekt NTS-1 digital kit      | v1.1.0             | >= v1.02                 | ARM Cortex-M4 | Custom 32-bit LSB executable, ARM, EABI5 v1 (SYSV), static  |
| drumlogue                      | v2.0.0             | >= v1.0.0                | ARM Cortex-A7 | ELF 32-bit LSB shared object, ARM, EABI5 v1 (SYSV), dynamic |
| Nu:Tekt NTS-1 digital kit mkII | v2.0.0             | >= v1.0.0                | ARM Cortex-M7 | ELF 32-bit LSB shared object, ARM, EABI5 v1 (SYSV), dynamic |
| Nu:Tekt NTS-3 kaoss pad kit    | v2.0.0             | >= v1.0.0                | ARM Cortex-M7 | ELF 32-bit LSB shared object, ARM, EABI5 v1 (SYSV), dynamic |
| microKORG2                     | v2.1.0             | >= v2.0.0                | ARM Cortex-A7 | ELF 32-bit LSB shared object, ARM, EABI5 v1 (SYSV), dynamic |

#### Binary Compatibility

User units built for prologue, minilogue xd and Nu:Tekt NTS-1 digital kit (mkI only) are binary compatible with each other, as long as the SDK version matches.
However, developers are encouraged to optimize their units for each target platform, and thus specialized builds should be preferred if available.

## Repository Structure

* [platform/prologue/](platform/prologue/) : *prologue* specific files, templates and demo projects.
* [platform/minilogue-xd/](platform/minilogue-xd/) : *minilogue xd* specific files, templates and demo projects.
* [platform/nutekt-digital/](platform/nutekt-digital/) : *Nu:Tekt NTS-1 digital kit* specific files, templates and demo projects.
* [platform/drumlogue/](platform/drumlogue/) : *drumlogue* specific files and templates.
* [platform/nts-1_mkii/](platform/nts-1_mkii/) : *Nu:Tekt NTS-1 digital kit mkII* specific files, templates and demo projects.
* [platform/nts-3_kaoss/](platform/nts-3_kaoss/) : *Nu:Tekt NTS-3 kaoss pad kit* specific files, templates and demo projects.
* [platform/microkorg2/](platform/microkorg2/) : *microKORG2* specific files, templates and demo projects.
* [platform/ext/](platform/ext/) : External dependencies and submodules.
* [docker/](docker/) : Sources for a docker container that allows building projects for any platform in a more host OS agnostic way.
* [tools/](tools/) : Installation location and documentation for tools required to build projects and manipulate built products. Can be ignored if using the docker container.
* [devboards/](devboards/) : Information and files related to limited edition development boards.

## Sharing your Oscillators/Effects with us

To show us your work please reach out to *logue-sdk@korg.co.jp*.

## Support

The SDK is provided as-is, no technical support will be provided by KORG.
For community support, please visit our discord at https://discord.com/invite/pqW5VpdQkZ


