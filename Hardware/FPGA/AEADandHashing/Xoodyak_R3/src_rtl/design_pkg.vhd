-------------------------------------------------------------------------------------------------
--! @file       Design_pkg.vhd
--! @brief      Package for the Xoodyak Crypto Core.
--!
--! @author     Silvia Mella <silvia.mella@st.com>
--! @license    To the extent possible under law, the implementer has waived all copyright
--!             and related or neighboring rights to the source code in this file.
--!             http://creativecommons.org/publicdomain/zero/1.0/
--! @note       This code is based on the package for the dummy cipher provided within 
--!             the Development Package for Hardware Implementations Compliant with 
--!             the Hardware API for Lightweight Cryptography (https://github.com/GMUCERG/LWC)
-------------------------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;

package Design_pkg is

    --! This module implements Xoodyak with n = 1, 2, 3, 4, 6, or 12 rounds per clock cycle.
    --! Specify the number of rounds per clock cycle by setting the constant roundsPerCycle to a valid value.
    --! Valid values are: 1, 2, 3, 4, 6, and 12
    constant roundsPerCycle : integer := 3;

--------------------------------------------------------------------------------
------------------------- DO NOT CHANGE ANYTHING BELOW -------------------------
--------------------------------------------------------------------------------
    -- asynchronous reset active high
    constant active_rst_p : std_logic := '1';

    --! design parameters needed by the PreProcessor, PostProcessor, and LWC; assigned in the package body below!
    
    constant TAG_SIZE        : integer; --! Tag size
    constant HASH_VALUE_SIZE : integer; --! Hash value size
    
    constant CCSW            : integer; --! variant dependent design parameter!
    constant CCW             : integer; --! variant dependent design parameter!
    constant CCWdiv8         : integer; --! derived from parameters above, assigned in body.
    
    --! design parameters specific to the CryptoCore; assigned in the package body below!
   
    constant NPUB_SIZE       : integer; --! Npub size
    constant DBLK_SIZE       : integer; --! Block size
    constant KEY_SIZE        : integer; --! Key size
    constant STATE_SIZE      : integer;
	constant RKIN            : integer; --! R_kin
	constant RKOUT           : integer; --! R_kout
    --constant LANE_SIZE       : integer;    
   	constant RHASH           : integer; --! R_hash
	--constant LRATCHET        : integer; --! l_ratchet
	--constant PHASE_UP        : std_logic;
	--constant PHASE_DOWN      : std_logic; 
    
    constant CMD_01         : std_logic_vector(7 downto 0); -- 0x01;
    constant CMD_ZERO       : std_logic_vector(7 downto 0); -- 0x00;
	constant CMD_ABSORB_KEY : std_logic_vector(7 downto 0); -- 0x02;
	constant CMD_ABSORB     : std_logic_vector(7 downto 0); -- 0x03;
	constant CMD_RATCHET    : std_logic_vector(7 downto 0); -- 0x10;
	constant CMD_SQUEEZE    : std_logic_vector(7 downto 0); -- 0x40;
	constant CMD_CRYPT      : std_logic_vector(7 downto 0); -- 0x80;
	--constant CMD_SQUEEZE_KEY : std_logic_vector(7 downto 0); -- 0x20;
    
    constant PADD_01          : std_logic_vector;-- 0x01;
    constant PADD_01_KEY      : std_logic_vector;-- 0x0100;
    constant PADD_01_KEY_NONCE : std_logic_vector; --0x0110;
                         
    constant DOMAIN_ABSORB_HASH : std_logic_vector;                         
    constant DOMAIN_ZERO        : std_logic_vector; 
    constant DOMAIN_ABSORB_KEY  : std_logic_vector; 
    constant DOMAIN_ABSORB      : std_logic_vector; 
    constant DOMAIN_RATCHET     : std_logic_vector; 
    constant DOMAIN_SQUEEZE     : std_logic_vector; 
    constant DOMAIN_CRYPT       : std_logic_vector;
    --constant DOMAIN_SQUEEZE_KEY : std_logic_vector;  

    --! Functions declaration
    
    --! Calculate the number of I/O words for a particular size
    function get_words(size: integer; iowidth:integer) return integer; 
    --! Calculate log2 and round up.
    function log2_ceil (N: natural) return natural;
    --! Reverse the Byte order of the input word.
    function reverse_byte( vec : std_logic_vector ) return std_logic_vector;
    --! Reverse the Bit order of the input vector.
    function reverse_bit( vec : std_logic_vector ) return std_logic_vector;
    --! Padding the current word.
    function padd( bdi, bdi_valid_bytes, bdi_pad_loc : std_logic_vector ) return std_logic_vector;
    --! Return max value
    function max( a, b : integer) return integer;
    
    --! Derive domain word from domain byte
    function domain_word( CMD : std_logic_vector ) return std_logic_vector;
    --! Calculate selection vector for the valid bytes
    function select_bytes( bdi, bdi_valid_bytes : std_logic_vector) return std_logic_vector;

end Design_pkg;


package body Design_pkg is

    --! design parameters needed by the PreProcessor, PostProcessor, and LWC
    constant TAG_SIZE        : integer := 128; --! Tag size
    constant HASH_VALUE_SIZE : integer := 256; --! Hash value size
    constant CCW             : integer := 32; --vector_of_constants(1); --! bdo/bdi width
    constant CCSW            : integer := 32; --vector_of_constants(2); --! key width
    constant CCWdiv8         : integer := CCW/8; -- derived from parameters above

    --! design parameters specific to the CryptoCore
    constant NPUB_SIZE       : integer := 128; --96 --! Npub size
    constant DBLK_SIZE       : integer := 352; --! Block size
    
    constant KEY_SIZE        : integer := 128; --! Key size
    constant STATE_SIZE      : integer := 384; --! State size
	constant RKIN            : integer := 352; --! R_kin
	constant RKOUT           : integer := 192; --! R_kout
    -- constant LANE_SIZE       : integer :=  32; -- lane size   
   	constant RHASH           : integer := 128; --! R_hash
	-- constant LRATCHET        : integer := 128; --! l_ratchet
	-- constant PHASE_UP        : std_logic := '1';
	-- constant PHASE_DOWN      : std_logic := '0'; 
    
    constant CMD_01         : std_logic_vector(7 downto 0) := X"01"; -- 0x01;
    constant CMD_ZERO       : std_logic_vector(7 downto 0) := X"00"; -- 0x00;
	constant CMD_ABSORB_KEY : std_logic_vector(7 downto 0) := X"02"; -- 0x02;
	constant CMD_ABSORB     : std_logic_vector(7 downto 0) := X"03"; -- 0x03;
	constant CMD_RATCHET    : std_logic_vector(7 downto 0) := X"10"; -- 0x10;
	constant CMD_SQUEEZE    : std_logic_vector(7 downto 0) := X"40"; -- 0x40;
	constant CMD_CRYPT      : std_logic_vector(7 downto 0) := X"80"; -- 0x80; 
	--constant CMD_SQUEEZE_KEY : std_logic_vector(7 downto 0) := X"20"; -- 0x20;   
    
    constant PADD_01     : std_logic_vector(CCW-1 downto 0) := (CCW-1 downto 1 => '0') & '1'; -- 0x01;
    constant PADD_01_KEY : std_logic_vector(CCW-1 downto 0) := (CCW-1 downto 9 => '0') & '1' & (7 downto 0 => '0'); -- 0x0100;
    constant PADD_01_KEY_NONCE : std_logic_vector(CCW-1 downto 0) := (CCW-1 downto 9 => '0') & '1' & (7 downto 5 => '0') & '1' & (3 downto 0 => '0'); -- 0x0110;
      
    
    --! Calculate the number of words
    function get_words(size: integer; iowidth:integer) return integer is
    begin
        if (size mod iowidth) > 0 then
            return size/iowidth + 1;
        else
            return size/iowidth;
        end if;
    end function get_words;

    --! Log of base 2
    function log2_ceil (N: natural) return natural is
    begin
         if ( N = 0 ) then
             return 0;
         elsif N <= 2 then
             return 1;
         else
            if (N mod 2 = 0) then
                return 1 + log2_ceil(N/2);
            else
                return 1 + log2_ceil((N+1)/2);
            end if;
         end if;
    end function log2_ceil;

    --! Reverse the Byte order of the input word.
    function reverse_byte( vec : std_logic_vector ) return std_logic_vector is
        variable res : std_logic_vector(vec'length - 1 downto 0);
        constant n_bytes  : integer := vec'length/8;
    begin

        -- Check that vector length is actually byte aligned.
        assert (vec'length mod 8 = 0)
            report "Vector size must be in multiple of Bytes!" severity failure;

        -- Loop over every byte of vec and reorder it in res.
        for i in 0 to (n_bytes - 1) loop
            res(8*(i+1) - 1 downto 8*i) := vec(8*(n_bytes - i) - 1 downto 8*(n_bytes - i - 1));
        end loop;

        return res;
    end function reverse_byte;

    --! Reverse the Bit order of the input vector.
    function reverse_bit( vec : std_logic_vector ) return std_logic_vector is
        variable res : std_logic_vector(vec'length - 1 downto 0);
    begin

        -- Loop over every bit in vec and reorder it in res.
        for i in 0 to (vec'length - 1) loop
            res(i) := vec(vec'length - i - 1);
        end loop;

        return res;
    end function reverse_bit;

    --! Padd the data with 0x01 Byte if pad_loc is set.
    function padd( bdi, bdi_valid_bytes, bdi_pad_loc : std_logic_vector) return std_logic_vector is
        variable res : std_logic_vector(bdi'length - 1 downto 0) := (others => '0');
    begin

        for i in 0 to (bdi_valid_bytes'length - 1) loop
            if (bdi_valid_bytes(i) = '1') then
                res(8*(i+1) - 1 downto 8*i) := bdi(8*(i+1) - 1 downto 8*i);
            elsif (bdi_pad_loc(i) = '1') then
                --res(8*(i+1) - 1 downto 8*i) := x"80";
                res(8*(i+1) - 1 downto 8*i) := x"01";
            end if;
        end loop;

        return res;
    end function;

    --! Return max value.
    function max( a, b : integer) return integer is
    begin
        if (a >= b) then
            return a;
        else
            return b;
        end if;
    end function;
    
    --! Derive domain word from domain byte    
    function domain_word( CMD : std_logic_vector ) return std_logic_vector is
        variable res : std_logic_vector(CCW - 1 downto 0) := (others => '0');
    begin 
        res(CCW - 1 downto CCW - 8) := CMD;
        return res;
    end function domain_word;
    
    --! Calculate selection vector for the valid bytes
    function select_bytes( bdi, bdi_valid_bytes : std_logic_vector) return std_logic_vector is
        variable res : std_logic_vector(bdi'length - 1 downto 0) := (others => '0');
    begin

        for i in 0 to (bdi_valid_bytes'length - 1) loop
            if (bdi_valid_bytes(i) = '1') then
                res(8*(i+1) - 1 downto 8*i) := (others => '0');
            else 
                res(8*(i+1) - 1 downto 8*i) := (others => '1');
            end if;
        end loop;

        return res;
    end function;
    
    constant DOMAIN_ABSORB_HASH : std_logic_vector(CCW-1 downto 0) := domain_word(X"01"); -- 0x00;
    constant DOMAIN_ZERO        : std_logic_vector(CCW-1 downto 0) := domain_word(X"00"); -- 0x00;
	constant DOMAIN_ABSORB_KEY  : std_logic_vector(CCW-1 downto 0) := domain_word(X"02"); -- 0x02;
	constant DOMAIN_ABSORB      : std_logic_vector(CCW-1 downto 0) := domain_word(X"03"); -- 0x03;
	constant DOMAIN_RATCHET     : std_logic_vector(CCW-1 downto 0) := domain_word(X"10"); -- 0x10;
	--constant DOMAIN_SQUEEZE_KEY : std_logic_vector(CCW-1 downto 0) := domain_word(X"20"); -- 0x20;
	constant DOMAIN_SQUEEZE     : std_logic_vector(CCW-1 downto 0) := domain_word(X"40"); -- 0x40;
	constant DOMAIN_CRYPT       : std_logic_vector(CCW-1 downto 0) := domain_word(X"80"); -- 0x80;

end package body Design_pkg;
