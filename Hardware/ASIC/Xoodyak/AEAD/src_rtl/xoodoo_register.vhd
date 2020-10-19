--------------------------------------------------------------------------------
--! @file       xoodoo_register.vhd
--! @brief      Xoodoo state.
--!
--! @author     Guido Bertoni
--! @author     Silvia Mella <silvia.mella@st.com>
--!
--! @license    To the extent possible under law, the implementer has waived all copyright
--!             and related or neighboring rights to the source code in this file.
--!             http://creativecommons.org/publicdomain/zero/1.0/
--------------------------------------------------------------------------------

library work;
    use work.xoodoo_globals.all;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;


entity xoodoo_register is
    port (
        clk             : in std_logic;
        rst             : in std_logic;
        init            : in std_logic;
        state_in        : in  x_state_type;
        state_out       : out x_state_type;
        word_in         : in std_logic_vector(31 downto 0);
        word_index_in   : in integer range 0 to 11;
        word_enable_in  : in std_logic;
        start_in        : in std_logic;
        running_in      : in std_logic;
        domain_i        : in std_logic_vector(31 downto 0);
        domain_enable_i : in std_logic;
        word_out        : out std_logic_vector(31 downto 0)
    );
end xoodoo_register;

architecture rtl of xoodoo_register is

----------------------------------------------------------------------------
-- Internal signal declarations
----------------------------------------------------------------------------

signal reg_value : x_state_type;

begin  -- Rtl

    begReg: process(clk,rst)
    begin
        if (rst = active_rst) then
            reg_value(0)(0) <= (others => '0');
            reg_value(0)(1) <= (others => '0');
            reg_value(0)(2) <= (others => '0');
            reg_value(0)(3) <= (others => '0');
            reg_value(1)(0) <= (others => '0');
            reg_value(1)(1) <= (others => '0');
            reg_value(1)(2) <= (others => '0');
            reg_value(1)(3) <= (others => '0');
            reg_value(2)(0) <= (others => '0');
            reg_value(2)(1) <= (others => '0');
            reg_value(2)(2) <= (others => '0');
            reg_value(2)(3) <= (others => '0');
        elsif rising_edge( clk ) then
            if(init = '1') then
                reg_value(0)(0) <= (others => '0');
                reg_value(0)(1) <= (others => '0');
                reg_value(0)(2) <= (others => '0');
                reg_value(0)(3) <= (others => '0');
                reg_value(1)(0) <= (others => '0');
                reg_value(1)(1) <= (others => '0');
                reg_value(1)(2) <= (others => '0');
                reg_value(1)(3) <= (others => '0');
                reg_value(2)(0) <= (others => '0');
                reg_value(2)(1) <= (others => '0');
                reg_value(2)(2) <= (others => '0');
                reg_value(2)(3) <= (others => '0');
            elsif (running_in = '1' or start_in = '1') then
                reg_value <= state_in;
            else
                if(domain_enable_i = '1') then
                    reg_value(2)(3) <= reg_value(2)(3) xor domain_i;
                end if;
                if(word_enable_in = '1') then
                    reg_value(word_index_in/4)(word_index_in mod 4) <= reg_value(word_index_in/4)(word_index_in mod 4) xor word_in;
                end if;
            end if;
        end if;
    end process begReg;

state_out <= reg_value;
word_out <= reg_value(word_index_in/4)(word_index_in mod 4);

end rtl;
