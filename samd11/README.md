Dapper Miser: SAMD11 variant
============================

## Introduction

[Dapper Miser](https://github.com/majbthrd/DapperMiser/) is a lightweight implementation of [CMSIS-DAP](http://arm-software.github.io/CMSIS_5/DAP/html/index.html) that can be used on low-cost USB microcontrollers.

This variant adapts the [free-dap](https://github.com/ataradov/free-dap) implementation, leaving its USB stack but substituting its CMSIS-DAP implementation with Dapper Miser's.

## Build Requirements

One approach is to use [Rowley Crossworks for ARM](http://www.rowley.co.uk/arm/) to compile this code.  It is not free software, but has been my favorite go-to ARM development tool for a decade and counting.

*OR*

Use the Makefile in the make subdirectory.  With this approach, the code can be built using only open-source software.  In Ubuntu-derived distributions, this is likely achieved with as little as:

```
sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

# Customizing

usb_descriptors.c contains the USB VID:PID.  All unique USB device implementations must have their own unique USB VID:PID identifiers.

swdio_bsp.h must be customized to reflect the choice of GPIO pins made in your hardware design.  As is, the file reflects the pin choices used in the [free-dap](https://github.com/ataradov/free-dap) "mini" adapter schematic.
