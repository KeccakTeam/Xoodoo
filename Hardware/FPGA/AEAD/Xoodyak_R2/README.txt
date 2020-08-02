Hardware Design Group: Xoodyak Team + Silvia
Primary Hardware Designers: Silvia Mella, silvia.mella@st.com
Academic advisors/Program managers: -
LWC candidate: Xoodyak

Xoodyak Hardware Implementation
===============================

This is a hardware implementation of Xoodyak authenticated encryption compliant with the Hardware API for Lightweight Cryptography.
The implementation does not include hash functionality.


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
