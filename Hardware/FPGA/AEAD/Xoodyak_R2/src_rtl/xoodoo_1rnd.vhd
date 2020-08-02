--------------------------------------------------------------------------------
--! @file       xoodoo_1rnd.vhd
--! @brief      Xoodoo permutation with 1 round per clock cycle.
--!
--! @author     Guido Bertoni
--! @author     Silvia Mella <silvia.mella@st.com>
--! @license    To the extent possible under law, the implementer has waived all copyright
--!             and related or neighboring rights to the source code in this file.
--!             http://creativecommons.org/publicdomain/zero/1.0/
--------------------------------------------------------------------------------




library work;
    use work.xoodoo_globals.all;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;

entity xoodoo_1rnd is
    generic( roundPerCycle : integer  := 1);
    port (
        clk_i           : in std_logic;
        rst_i           : in std_logic;
        start_i         : in std_logic;
        state_i         : in std_logic_vector(383 downto 0);  --in  x_state_type;
        state_o         : out std_logic_vector(383 downto 0); --out x_state_type;
        state_valid_o   : out std_logic
    );
end xoodoo_1rnd;

architecture rtl of xoodoo_1rnd is

    --components

    component xoodoo_n_rounds
        generic( roundPerCycle : integer  := 1);
        port (
            state_in     : in  x_state_type;
            state_out    : out x_state_type;
            rc_state_in  : in std_logic_vector(5 downto 0);
            rc_state_out : out std_logic_vector(5 downto 0)
        );
    end component;

    component xoodoo_register
        port (
            clk         : in std_logic;
            rst         : in std_logic;
            state_in    : in  x_state_type;
            state_out   : out x_state_type
        );
    end component;

      ----------------------------------------------------------------------------
      -- Internal signal declarations
      ----------------------------------------------------------------------------

    -- round constants

    signal round_in,round_out,reg_in,reg_out : x_state_type;
    signal rc_state_in, rc_state_out : std_logic_vector(5 downto 0);
    signal done,running : std_logic;
    
begin  -- rtl

    rg00_map : xoodoo_register 
        port map(
            clk       => clk_i,
            rst       => rst_i,
            state_in  => reg_in,
            state_out => reg_out
        );

    rd00_map : xoodoo_n_rounds 
        generic map (roundPerCycle => roundPerCycle)
        port map(
            state_in     => round_in,
            state_out    => round_out,
            rc_state_in  => rc_state_in,
            rc_state_out => rc_state_out
        );

    main: process(clk_i)
    begin
        if rising_edge(clk_i) then
            if (rst_i = active_rst) then
                done <= '0';
                running <= '0';
                --rc_state_in <= "100011";
                rc_state_in <= "011011";
            else
                if start_i='1' then
                    done <= '0';
                    running <= '1';
                    rc_state_in <= rc_state_out;
                    --rc_state_in <= "011011";
                elsif rc_state_out = "010011" then
                    done <= '1';
                    running <= '0'; 
                    --rc_state_in <= "100011";
                    rc_state_in <= "011011";
                elsif running = '1' then
                    done <= '0';
                    running <= '1'; 
                    rc_state_in <= rc_state_out;
                end if;
            end if;
        end if;
    end process;

    round_in <= stdlogicvector_to_xstate(state_i) when running='0' else 
    --round_in <= stdlogicvector_to_xstate(state_i) when (running = '0' or start_i = '1') else --state_i when (running = '0' or start_i = '1') else
                reg_out;

    reg_in <= round_out when (running or start_i) = '1' else 
                ZERO_STATE;

    state_o <= xstate_to_stdlogicvector(reg_out); --reg_out; 

    state_valid_o <= done;
  
end rtl;