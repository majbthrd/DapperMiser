Dapper Miser: lightweight CMSIS-DAP implementation
==================================================

In late 2013, I created a functional implementation of CMSIS-DAP called "Dapper Miser" that runs in a low-cost USB microcontroller such as the PIC16F1454. My interest was seeing it deployed as a CMSIS-DAP debugger *integrated* into a low-cost hobbyist/educational ARM development board.

From 2013 to 2017, ARM only made the CMSIS-DAP specification available under a EULA ([defunct link](https://silver.arm.com/browse/CMSISDAP)); this precluded anyone from releasing an open-source implementation that honored the EULA.  However, sometime in 2017, ARM changed their tune and published (devoid of a EULA) a CMSIS-DAP specification [here](http://arm-software.github.io/CMSIS_5/DAP/html/index.html).

What distinguishes Dapper Miser from ARM's reference implementation (and its assorted clones) is that Dapper Miser's architecture was optimized to have a lightweight program footprint.  This made it possible to implement CMSIS-DAP on an 8-bit microcontroller with far less resources than what ARM says is necessary.
Please read the [app note](./appnote/README.md) for more information on the implementation, and the associated README.md with each processor target.
