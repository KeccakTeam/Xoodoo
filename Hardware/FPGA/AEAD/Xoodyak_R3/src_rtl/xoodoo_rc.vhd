--------------------------------------------------------------------------------
--! @file       xoodoo_rc.vhd
--! @brief      Xoodoo round constants computation.
--!
--! @author     Guido Bertoni
--! @license    To the extent possible under law, the implementer has waived all copyright
--!             and related or neighboring rights to the source code in this file.
--!             http://creativecommons.org/publicdomain/zero/1.0/
--------------------------------------------------------------------------------

library work;
    use work.xoodoo_globals.all;

library ieee;
    use ieee.std_logic_1164.all;
    use ieee.numeric_std.all;


entity xoodoo_rc is
port (
    state_in  : in  std_logic_vector(5 downto 0);
    state_out : out std_logic_vector(5 downto 0);
    rc        : out std_logic_vector(31 downto 0)
    );
end xoodoo_rc;

architecture behavioral of xoodoo_rc is

    signal si : std_logic_vector(2 downto 0);
    signal temp_new_si : unsigned(3 downto 0);
    signal new_si : unsigned(2 downto 0);

    signal qi : std_logic_vector(2 downto 0);
    signal new_qi : std_logic_vector(2 downto 0);

    signal qi_plus_t3 : std_logic_vector(3 downto 0);

    begin

    si <= state_in(2 downto 0);
    temp_new_si <= resize(unsigned(si), 4) + unsigned(si(1 downto 0) & si(2));
    new_si <= temp_new_si(2 downto 0) + temp_new_si(3 downto 3);
    state_out(2 downto 0) <= std_logic_vector(new_si);

    qi <= state_in(5 downto 3);
    new_qi(0) <= qi(2);
    new_qi(1) <= qi(0) xor qi(2);
    new_qi(2) <= qi(1);
    state_out(5 downto 3) <= new_qi;

    qi_plus_t3(0) <= qi(0);
    qi_plus_t3(1) <= qi(1);
    qi_plus_t3(2) <= qi(2);
    qi_plus_t3(3) <= '1';

    process(si, qi_plus_t3)
    begin
        rc(31 downto 10) <= (others => '0');
        case si is
            when "001" =>
                rc(0) <= '0';
                rc(1) <= qi_plus_t3(0);
                rc(2) <= qi_plus_t3(1);
                rc(3) <= qi_plus_t3(2);
                rc(4) <= qi_plus_t3(3);
                rc(9 downto 5) <= (others => '0');
            when "010" =>
                rc(1 downto 0) <= (others => '0');
                rc(2) <= qi_plus_t3(0);
                rc(3) <= qi_plus_t3(1);
                rc(4) <= qi_plus_t3(2);
                rc(5) <= qi_plus_t3(3);
                rc(9 downto 6) <= (others => '0');
            when "011" =>
                rc(2 downto 0) <= (others => '0');
                rc(3) <= qi_plus_t3(0);
                rc(4) <= qi_plus_t3(1);
                rc(5) <= qi_plus_t3(2);
                rc(6) <= qi_plus_t3(3);
                rc(9 downto 7) <= (others => '0');
            when "100" =>
                rc(3 downto 0) <= (others => '0');
                rc(4) <= qi_plus_t3(0);
                rc(5) <= qi_plus_t3(1);
                rc(6) <= qi_plus_t3(2);
                rc(7) <= qi_plus_t3(3);
                rc(9 downto 8) <= (others => '0');
            when "101" =>
                rc(4 downto 0) <= (others => '0');
                rc(5) <= qi_plus_t3(0);
                rc(6) <= qi_plus_t3(1);
                rc(7) <= qi_plus_t3(2);
                rc(8) <= qi_plus_t3(3);
                rc(9) <= '0';
            when "110" =>
                rc(5 downto 0) <= (others => '0');
                rc(6) <= qi_plus_t3(0);
                rc(7) <= qi_plus_t3(1);
                rc(8) <= qi_plus_t3(2);
                rc(9) <= qi_plus_t3(3);
            when others =>
                -- This could be don't care.
                rc(9 downto 0) <= (others => '0');
        end case;
    end process;

end behavioral;