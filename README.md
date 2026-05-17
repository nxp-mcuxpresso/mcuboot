# MCUboot in MCUXpresso SDK

## Overview
This repository is a fork of [MCUboot](https://github.com/mcu-tools/mcuboot) for MCUXpresso SDK delivery and it contains the components officially provided in NXP MCUXpresso SDK. This repository is part of the MCUXpresso SDK overall delivery which is composed of several sub-repositories/projects. Navigate to the top/parent repository (mcuxsdk-manifests) for the complete delivery of MCUXpresso SDK.

MCUboot in MCUXpresso SDK is a fork of the original project that aims to integrate HW features that haven't yet been covered by the upstream code. Currently these are:
 - flash remapping techniques for zero-overhead image swapping
 - support for encrypted XIP
 - use of NXP's SB3 file

## Documentation
- Overall details of MCUXpresso SDK can be reviewed here: [MCUXpresso SDK Online Documentation](https://mcuxpresso.nxp.com/mcuxsdk/latest/html/introduction/README.html)  
- MCUXpresso SDK examples based on MCUboot are described here: [MCUXpresso SDK OTA Examples](https://mcuxpresso.nxp.com/mcuxsdk/latest/html/examples/ota_examples/mcuboot_opensource/readme.html)  
- Visit [MCUboot - Documentation](https://docs.mcuboot.com/) to review details on the original contents in this sub-repo

## Setup
Instructions on how to install the MCUXpresso SDK provided from GitHub via west manifest [Getting Started with SDK - Detailed Installation Instructions](https://mcuxpresso.nxp.com/mcuxsdk/latest/html/gsd/installation.html#installation)

## Contribution
Contributions are not currently accepted.
If the intended contribution is not related to NXP specific code, consider contributing directly to the upstream MCUboot project. Once this MCUboot fork is synchronized with the upstream project, such contributions will end up here as well.
If the intended contribution is a bugfix or improvement for NXP porting layer or for code added or modified by NXP, please open an issue or contact NXP support.

