------------------------------------------------------------------------------
--! @file       NIST_LWAPI_pkg.vhd 
--! @brief      NIST lightweight API package
--! @author     Panasayya Yalla & Ekawat (ice) Homsirikamol
--! @copyright  Copyright (c) 2016 Cryptographic Engineering Research Group
--!             ECE Department, George Mason University Fairfax, VA, U.S.A.
--!             All rights Reserved.
--! @license    This project is released under the GNU Public License.
--!             The license and distribution terms for this file may be
--!             found in the file LICENSE in this distribution or at
--!             http://www.gnu.org/licenses/gpl-3.0.txt
--! @note       This is publicly available encryption source code that falls
--!             under the License Exception TSU (Technology and software-
--!             unrestricted)
--------------------------------------------------------------------------------
--! Description
--!
--!
--!
--!
--!
--!
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;

package NIST_LWAPI_pkg is

    --! External bus: supported values are 8, 16 and 32 bits
    constant W       : integer :=32;
    constant SW      : integer :=W;

    --! Default values for do_data bus
    --! to avoid leaking intermeadiate values if do_valid = '0'.
    constant do_data_defaults : std_logic_vector(W-1 downto 0) := (others => '0');

    -- DO NOT CHANGE ANYTHING BELOW!
    constant Wdiv8   : integer := W/8;
    constant SWdiv8  : integer := SW/8;

    --! INSTRUCTIONS (OPCODES)
    constant INST_HASH      : std_logic_vector(4    -1 downto 0):="1000";
    constant INST_ENC       : std_logic_vector(4    -1 downto 0):="0010";
    constant INST_DEC       : std_logic_vector(4    -1 downto 0):="0011";
    constant INST_LDKEY     : std_logic_vector(4    -1 downto 0):="0100";
    constant INST_ACTKEY    : std_logic_vector(4    -1 downto 0):="0111";
    constant INST_SUCCESS   : std_logic_vector(4    -1 downto 0):="1110";
    constant INST_FAILURE   : std_logic_vector(4    -1 downto 0):="1111";
    --! SEGMENT TYPE ENCODING
    --! Reserved                                                :="0000";
    constant HDR_AD         : std_logic_vector(4    -1 downto 0):="0001";
    constant HDR_NPUB_AD    : std_logic_vector(4    -1 downto 0):="0010";
    constant HDR_AD_NPUB    : std_logic_vector(4    -1 downto 0):="0011";
    constant HDR_PT         : std_logic_vector(4    -1 downto 0):="0100";
        --deprecated! use HDR_PT instead!
       alias HDR_MSG is                                           HDR_PT;
    constant HDR_CT         : std_logic_vector(4    -1 downto 0):="0101";
    constant HDR_CT_TAG     : std_logic_vector(4    -1 downto 0):="0110";
    constant HDR_HASH_MSG   : std_logic_vector(4    -1 downto 0):="0111";
    constant HDR_TAG        : std_logic_vector(4    -1 downto 0):="1000";
    constant HDR_HASH_VALUE : std_logic_vector(4    -1 downto 0):="1001";
    --NOT USED in this support package
    constant Length         : std_logic_vector(4    -1 downto 0):="1010";
    --! Reserved                                                :="1011";
    constant HDR_KEY        : std_logic_vector(4    -1 downto 0):="1100";
    constant HDR_NPUB       : std_logic_vector(4    -1 downto 0):="1101";
    --NOT USED in NIST LWC
    constant HDR_NSEC       : std_logic_vector(4    -1 downto 0):="1110";
     --NOT USED in NIST LWC
    constant HDR_ENSEC      : std_logic_vector(4    -1 downto 0):="1111";
    --! Maximum supported length
    --! Length of segment header
    constant SINGLE_PASS_MAX: integer := 16;
    --! Length of segment header
    constant TWO_PASS_MAX   : integer := 16;

    --! Other
    --! Limit to the segment counter size
    constant CTR_SIZE_LIM   : integer := 16;

    --! =======================================================================
    --! Deprecated parameters from CAESAR-LWAPI! DO NOT CHANGE!
    --! asynchronous reset is not supported
    constant ASYNC_RSTN   : boolean := FALSE;

    --! =======================================================================
    --! Functions used by LWC Core, PreProcessor and PostProcessor
    --! expands input vector 8 times.
    function Byte_To_Bits_EXP (bytes_in : std_logic_vector) return std_logic_vector;

end NIST_LWAPI_pkg;

package body NIST_LWAPI_pkg is

function Byte_To_Bits_EXP (
    bytes_in : std_logic_vector
    ) return std_logic_vector is

    variable bits : std_logic_vector ( (8* bytes_in'length) -1 downto 0);
begin

    for i in 0 to bytes_in'length-1 loop
        if (bytes_in(i) = '1') then
            bits(8*(i+1)-1 downto 8*i) := (others =>'1');
        else
            bits(8*(i+1)-1 downto 8*i) := (others =>'0');
        end if;
    end loop;

    return bits;
end Byte_To_Bits_EXP;

end NIST_LWAPI_pkg;
