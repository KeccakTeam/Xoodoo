--------------------------------------------------------------------------------
--! @file       xoodoo_n_rounds.vhd
--! @brief      Xoodoo rounds: n per cycle.
--!
--! @author     Guido Bertoni
--! @license    To the extent possible under law, the implementer has waived all copyright
--!             and related or neighboring rights to the source code in this file.
--!             http://creativecommons.org/publicdomain/zero/1.0/
--------------------------------------------------------------------------------

library work;
    use work.xoodoo_globals.all;
    use work.design_pkg.all;

library ieee;
    use ieee.std_logic_1164.all;
    use ieee.std_logic_arith.all;


entity xoodoo_n_rounds is
    generic( roundPerCycle : integer  := roundsPerCycle);
    port (
        state_in     : in  x_state_type;
        state_out    : out x_state_type;
        rc_state_in  : in std_logic_vector(5 downto 0);
        rc_state_out : out std_logic_vector(5 downto 0)
    );
end xoodoo_n_rounds;

architecture behavioral of xoodoo_n_rounds is

component xoodoo_round
    port (

        state_in     : in  x_state_type;
        rc           : in std_logic_vector(31 downto 0);
        state_out    : out x_state_type
    );
end component;

component xoodoo_rc
    port (
        state_in  : in  std_logic_vector(5 downto 0);
        state_out : out std_logic_vector(5 downto 0);
        rc : out std_logic_vector(31 downto 0)
    );
end component;

type state_rounds is array(integer range 0 to (roundPerCycle - 1)) of x_state_type;
type state_rc_rounds is array(integer range 0 to (roundPerCycle - 1)) of std_logic_vector(5 downto 0);
type rc_rounds is array(integer range 0 to (roundPerCycle - 1)) of std_logic_vector(31 downto 0);

signal xoodoo_state_in : state_rounds;
signal xoodoo_rc_value : rc_rounds;
signal xoodoo_state_out : state_rounds;

signal xoodoo_rc_state_in : state_rc_rounds;
signal xoodoo_rc_state_out : state_rc_rounds;

begin

xoodoo_state_in(0) <= state_in;
xoodoo_rc_state_in(0) <= rc_state_in;

ALL_ROUNDS : for I in state_rounds'range generate

    INPUT_STATE_FROM_PREVIOUS : if I > 0 generate

        xoodoo_state_in(I) <= xoodoo_state_out(I - 1);
        xoodoo_rc_state_in(I) <= xoodoo_rc_state_out(I - 1);

    end generate;

    rd_I : xoodoo_round
        port map(
            state_in => xoodoo_state_in(I),
            rc => xoodoo_rc_value(I),
            state_out => xoodoo_state_out(I)
        );

    rc_I : xoodoo_rc
        port map(
            state_in => xoodoo_rc_state_in(I),
            rc => xoodoo_rc_value(I),
            state_out => xoodoo_rc_state_out(I)
        );

end generate;

state_out <= xoodoo_state_out(roundPerCycle - 1);
rc_state_out <= xoodoo_rc_state_out(roundPerCycle - 1);

end behavioral;