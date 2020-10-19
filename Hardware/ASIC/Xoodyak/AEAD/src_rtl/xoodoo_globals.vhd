--------------------------------------------------------------------------------
--! @file       xoodoo_globals.vhd
--! @brief      Package for Xoodoo permutation.
--!
--! @author     Guido Bertoni
--! @license    To the extent possible under law, the implementer has waived all copyright
--!             and related or neighboring rights to the source code in this file.
--!             http://creativecommons.org/publicdomain/zero/1.0/
--------------------------------------------------------------------------------

library STD;
    use STD.textio.all;

library IEEE;
    use IEEE.std_logic_1164.all;
    use IEEE.std_logic_misc.all;
    use IEEE.std_logic_arith.all;

library work;


package xoodoo_globals is

    constant active_rst : std_logic;

    --types
    type x_plane_type is array (0 to 3) of std_logic_vector(31 downto 0);
    type x_state_type is array (0 to 2) of x_plane_type;

    function xstate_to_stdlogicvector(state_type_i : x_state_type)
                return std_logic_vector;

    function stdlogicvector_to_xstate(slv_i : std_logic_vector)
                return x_state_type;

    constant ZERO_STATE : x_state_type;

end package;

package body xoodoo_globals is

    constant active_rst : std_logic := '0';

    function xstate_to_stdlogicvector(state_type_i : x_state_type)
                return std_logic_vector is
    variable retval : std_logic_vector(383 downto 0);
    begin
    for y in 0 to 2 loop
      for x in 0 to 3 loop
        for i in 0 to 31 loop
          retval(128*y+32*x+i):=state_type_i(y)(x)(i);
        end loop;
      end loop;
    end loop;
    return retval;
    end xstate_to_stdlogicvector;

    function stdlogicvector_to_xstate(slv_i : std_logic_vector)
                return x_state_type is
    variable retval : x_state_type;
    begin
    for y in 0 to 2 loop
      for x in 0 to 3 loop
        for i in 0 to 31 loop
          retval(y)(x)(i):=slv_i(128*y+32*x+i);
        end loop;
      end loop;
    end loop;
    return retval;
    end stdlogicvector_to_xstate;

    constant ZERO_STATE : x_state_type := stdlogicvector_to_xstate(x"000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");

end package body;
