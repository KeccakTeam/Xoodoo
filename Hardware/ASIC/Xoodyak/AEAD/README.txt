Hardware Design Group: Xoodyak Team + Silvia
Primary Hardware Designers: Silvia Mella, silvia.mella@st.com
LWC candidate: Xoodyak (AEAD, no hashing)

Xoodyak Hardware Implementation
===============================

This is a hardware implementation of Xoodyak authenticated encryption, compliant with the Hardware API for Lightweight Cryptography.

Notes
--------------------------------------------------------

The implementation does not include hash functionality.
The implementation makes use of the Development Package for Hardware Implementations Compliant with the Hardware API for Lightweight Cryptography.

!!!!!!!!!!!!!!!!
The implementation allows to configure (at design time) the number of rounds that are computed in a single clock cycle.
Such number can be configured by setting the constant 
    roundsPerCycle
in configuration file src_rtl/design_pkg.vhd.
The constant roundsPerCycle can be set to any integer divisor of 12, i.e. 1,2,3,4,6 or 12.
!!!!!!!!!!!!!!!!



Design goal: 
--------------------------------------------------------

Trade-off between throughput and area



Formula to calculate throughput from the clock frequency
--------------------------------------------------------

The formula depends on the operation and on the value of the constant roundsPerCycle.
Let F denote the frequency.

Throughput for Encryption and Decryption: 192/(192/32+1+2+12/roundsPerCycle)*F
Throughput for Authentication           : 352/(352/32+1+2+12/roundsPerCycle)*F

Formulas above give the following throughput:
        
 Rounds per clock cycle | 
 (roundsPerCycle)       |  Encryption  |  Decryption  | Authentication 
=======================================================================
        1               |    9.143*F   |    9.143*F   |     13.539*F   
        2               |   12.800*F   |   12.800*F   |     17.600*F   
        3               |   14.769*F   |   14.769*F   |     19.556*F   
        4               |   16.000*F   |   16.000*F   |     20.706*F   
        6               |   17.455*F   |   17.455*F   |     22.000*F   
       12               |   19.200*F   |   19.200*F   |     23.467*F   



Expected area in gate equivalents
--------------------------------------------------------

 Rounds per clock cycle | Frequency |
 (roundsPerCycle)       |  100MHz   |
=====================================
        1               |    8400   |



Code avaialbility
--------------------------------------------------------

The code will soon be available on GitHub: https://github.com/KeccakTeam/Xoodoo
Please, link such repository instead of making the code available on https://github.com/mustafam001/lwc-aead-rtl



Folder structure
--------------------------------------------------------

This folder includes:

`$root/KAT`

Known-Answer-Test files folder.

`$root/src_rtl`

VHDL source code for Xoodyak.

* `./LWC`

    Files being a part of the Development Package for the LWC Hardware API.

`$root/src_tb`

Testbench (part of the Development Package for the LWC Hardware API).

`source_list.txt`

The list of files in a bottom-up approach.
