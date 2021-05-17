Hardware Design Group: Xoodyak Team + Silvia
Primary Hardware Designers: Silvia Mella, silvia.mella@st.com
LWC candidate: Xoodyak (AEAD only)

Xoodyak Hardware Implementation
===============================

This is a hardware implementation of Xoodyak authenticated encryption, compliant with the Hardware API for Lightweight Cryptography.



Changelog
---------
May 7, 2021
* Tweak for Round 3 of NIST LWC competition
  * Updated CryptoCore.vhd
  * Updated formula to calculate throughput
  * Updated KAT

Jan 10, 2021
* Source code: 
  * Updated testbench to version v1.1.0 of the Development Package for the LWC Hardware API
  * Modified CryptoCore.vhd to remove two clock cycles for each input block 

Oct 19, 2020
Initial version



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

Notation
Na, Nm, Nc : the number of complete blocks of associated data, plaintext, and ciphertext, respectively
Ina, Inm, Inc : binary variables equal to 1 if the last block of the respective data type is incomplete, and 0 otherwise
Bla, Blm, Blc : the number of bytes in the incomplete block of associated data, plaintext, and ciphertext, respectively.

Execution time of authenticated encryption:
15 + max(1,11*Na+Ina*ceil(Bla/4)) + (1+12/roundsPerCycle)*(max(1,Na+Ina)) + 6*Nm + Inm*ceil(Blm/4) + (1+12/roundsPerCycle)*(max(1,Nm+Inm)) + 4 + 12/roundsPerCycle

Execution time of authenticated decryption:
15 + max(1,11*Na+Ina*ceil(Bla/4)) + (1+12/roundsPerCycle)*(max(1,Na+Ina)) + 6*Nc + Inc*ceil(Blc/4) + (1+12/roundsPerCycle)*(max(1,Nc+Inc)) + 4 + 12/roundsPerCycle + 1

Formulas above give the following throughput for long messages.
Let F denote the frequency.

Throughput for Encryption and Decryption: 192/(192/32+1+12/roundsPerCycle)*F
Throughput for Authentication           : 352/(352/32+1+12/roundsPerCycle)*F

Formulas above give the following throughput:
        
 Rounds per clock cycle | 
 (roundsPerCycle)       |  Encryption  |  Decryption  | Authentication 
=======================================================================
        1               |   10.105*F   |   10.105*F   |     14.667*F   
        2               |   14.769*F   |   14.769*F   |     19.556*F   
        3               |   17.455*F   |   17.455*F   |     22.000*F   
        4               |   19.200*F   |   19.200*F   |     23.467*F   
        6               |   21.333*F   |   21.333*F   |     25.143*F   
       12               |   24.000*F   |   24.000*F   |     27.077*F   



Expected area in gate equivalents
--------------------------------------------------------

 Rounds per clock cycle | Frequency |
 (roundsPerCycle)       |  100MHz   |
=====================================
        1               |    7900   |



Code availability
--------------------------------------------------------

The code is available on GitHub: https://github.com/KeccakTeam/Xoodoo



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
