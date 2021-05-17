--------------------------------------------------------------------------------
--! @file       xoodoo.vhd
--! @brief      Xoodoo permutation with a given number of rounds per cycle.
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


entity xoodoo is
    generic( roundPerCycle : integer  := 1);
    port (
        clk_i           : in std_logic;
        rst_i           : in std_logic;
        start_i         : in std_logic;
        state_valid_o   : out std_logic;
        init_reg        : in std_logic;
        word_in         : in std_logic_vector(31 downto 0);
        word_index_in   : in integer range 0 to 11;
        word_enable_in  : in std_logic;
        domain_i        : in std_logic_vector(31 downto 0);
        domain_enable_i : in std_logic;
        word_out        : out std_logic_vector(31 downto 0)
    );
end xoodoo;

architecture rtl of xoodoo is

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
            init        : in std_logic;

            state_in    : in  x_state_type;
            state_out   : out x_state_type;

            word_in         : in std_logic_vector(31 downto 0);
            word_index_in   : in integer range 0 to 11;
            word_enable_in  : in std_logic;
            start_in        : in std_logic;
            running_in      : in std_logic;
            domain_i        : in std_logic_vector(31 downto 0);
            domain_enable_i : in std_logic;
            word_out        : out std_logic_vector(31 downto 0)
        );
    end component;

      ----------------------------------------------------------------------------
      -- Internal signal declarations
      ----------------------------------------------------------------------------

    -- round constants

    signal round_in,round_out,reg_in,reg_out : x_state_type;
    signal rc_state_in, rc_state_out : std_logic_vector(5 downto 0);
    signal done,running : std_logic;
    signal word_in_s : std_logic_vector(31 downto 0);
    signal word_index_in_s : integer range 0 to 11;
    signal word_enable_in_s : std_logic;
    signal init_reg_s : std_logic;
    signal domain_s : std_logic_vector(31 downto 0);
    signal domain_enable_s : std_logic;
    signal word_out_s : std_logic_vector(31 downto 0);

begin  -- rtl

    rg00_map : xoodoo_register
        port map(
            clk             => clk_i,
            rst             => rst_i,
            init            => init_reg_s,
            state_in        => reg_in,
            state_out       => reg_out,
            word_in         => word_in_s,
            word_index_in   => word_index_in_s,
            word_enable_in  => word_enable_in_s,
            start_in        => start_i,
            running_in      => running,
            domain_i        => domain_s,
            domain_enable_i => domain_enable_s,
            word_out        => word_out_s
        );

    rd00_map : xoodoo_n_rounds
        generic map (roundPerCycle => roundPerCycle)
        port map(
            state_in     => round_in,
            state_out    => round_out,
            rc_state_in  => rc_state_in,
            rc_state_out => rc_state_out
        );

    main: process(clk_i,rst_i)
    begin
        if (rst_i = active_rst) then
            done <= '0';
            running <= '0';
            --rc_state_in <= "100011";
            rc_state_in <= "011011";
        elsif rising_edge(clk_i) then
            if start_i='1' then
                done <= '0';
                running <= '1';
                rc_state_in <= rc_state_out;
                --rc_state_in <= "011011";
            elsif running = '1' then
                done <= '0';
                running <= '1';
                rc_state_in <= rc_state_out;
            end if;
            
            if rc_state_out = "010011" then
                done <= '1';
                running <= '0';
                --rc_state_in <= "100011";
                rc_state_in <= "011011";
            end if;
        end if;
    end process;

    round_in <= reg_out;
    reg_in <= round_out;
    state_valid_o <= done;
    word_in_s <= word_in;
    word_index_in_s <= word_index_in;
    word_enable_in_s <= word_enable_in;
    domain_s <= domain_i;
    domain_enable_s <= domain_enable_i;
    init_reg_s <= init_reg;
    word_out <= word_out_s;

end rtl;