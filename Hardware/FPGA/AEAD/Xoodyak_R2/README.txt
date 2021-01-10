Hardware Design Group: Xoodyak Team + Silvia
Primary Hardware Designers: Silvia Mella, silvia.mella@st.com
LWC candidate: Xoodyak (without hashing)

Xoodyak Hardware Implementation
===============================

This is a hardware implementation of Xoodyak authenticated encryption compliant with the Hardware API for Lightweight Cryptography.
The implementation does not include hash functionality.

Changelog
---------
Jan 10, 2021
* Source code: 
  * Updated LWC files and testbench to version v1.1.0 of the Development Package for the LWC Hardware API
  * Modified CryptoCore.vhd to remove two clock cycles for each input block 
* Docs
  * Updated formulas for the execution time
  * Updated latency according to measurement mode in LWC_TB testbench

Sep 2, 2020
Reduced area

Aug 2, 2020
Initial version

Folder structure
----------------

This folder includes:

`$root/docs`

Documents folder.

`$root/KAT`

Known-Answer-Test files folder.

`$root/src_rtl`

VHDL source code for Xoodyak.

* `./LWC`

    Files being a part of the Development Package for the LWC Hardware API.

`$root/src_tb`

Testbench (part of the Development Package for the LWC Hardware API).


Notes
----------------

The implementation does not include hash functionality.
The implementation makes use of the Development Package for Hardware Implementations Compliant with the Hardware API for Lightweight Cryptography.

The implementation allows to configure at design time the number of rounds that are computed in a single clock cycle.
Such number can be configured by setting the constant 
    roundsPerCycle
in configuration file src_rtl/design_pkg.vhd.
The constant roundsPerCycle can be set to any integer divisor of 12, i.e. 1,2,3,4,6 or 12.

KAT is the same for all configurations.
