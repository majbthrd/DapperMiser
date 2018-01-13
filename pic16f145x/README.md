Dapper Miser: PIC16F145x variant
================================

## Introduction

[Dapper Miser](https://github.com/majbthrd/DapperMiser/) is a lightweight implementation of [CMSIS-DAP](http://arm-software.github.io/CMSIS_5/DAP/html/index.html) that can be used on low-cost USB microcontrollers.

This variant is based on top of the [M-Stack USB driver stack by Alan Ott, Signal 11 Software](http://www.signal11.us/oss/m-stack/)

It is intended to be used with the [USB DFU Bootloader for PIC16F1454/5/9](https://github.com/majbthrd/PIC16F1-USB-DFU-Bootloader/).

# Customizing

usb_descriptors.c contains the USB VID:PID.  All unique USB device implementations must have their own unique USB VID:PID identifiers.

swdio_bsp.h must be customized to reflect any changes in choosing GPIO pins made in your hardware design.
