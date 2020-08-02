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
        clk         : in std_logic;
        rst         : in std_logic;
        state_in    : in  x_state_type;
        state_out   : out x_state_type
    );
end xoodoo_register;

architecture rtl of xoodoo_register is

----------------------------------------------------------------------------
-- Internal signal declarations
----------------------------------------------------------------------------

signal reg_value : x_state_type;

begin  -- Rtl

    begReg: process(clk)
    begin
        if rising_edge(clk) then
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
            else
                reg_value <= state_in;
            end if;
        end if;
    end process begReg;

state_out <= reg_value;

end rtl;
