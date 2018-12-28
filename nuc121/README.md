Dapper Miser: NUC121/NUC125 variant
===================================

## Introduction

[Dapper Miser](https://github.com/majbthrd/DapperMiser/) is a lightweight implementation of [CMSIS-DAP](http://arm-software.github.io/CMSIS_5/DAP/html/index.html) that can be used on low-cost USB microcontrollers.

This variant adopts the USB stack from [NUC121usb](https://github.com/majbthrd/NUC121usb/).

## Build Requirements

One approach is to use [Rowley Crossworks for ARM](http://www.rowley.co.uk/arm/) to compile this code.  It is not free software, but has been my favorite go-to ARM development tool for a decade and counting.  Rowley does not officially support the Nuvoton NUC121/NUC125, but you can [download an open-source CPU support package for the NUC121/NUC125](https://github.com/majbthrd/MCUmisfits/).

*OR*

Use the Makefile in the make subdirectory.  With this approach, the code can be built using only open-source software.  In Ubuntu-derived distributions, this is likely achieved with as little as:

```
sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

# Customizing

usb_descriptors.c contains the USB VID:PID.  All unique USB device implementations must have their own unique USB VID:PID identifiers.

swdio_bsp.h *MUST* be customized to reflect the choice of GPIO pins made in your hardware design.
