--------------------------------------------------------------------------------
--! @file       xoodoo_round.vhd
--! @brief      Xoodoo round function.
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
use ieee.std_logic_arith.all;


entity xoodoo_round is
port (
    state_in     : in  x_state_type;
    rc           : in std_logic_vector(31 downto 0);
    state_out    : out x_state_type
    );
end xoodoo_round;

architecture rtl of xoodoo_round is

  ----------------------------------------------------------------------------
  -- Internal signal declarations
  ----------------------------------------------------------------------------

signal theta_in,theta_out,rho_w_in,rho_w_out,rho_e_in,rho_e_out,iota_in,iota_out,chi_in,chi_out : x_state_type;
signal p,e: x_plane_type;

begin  -- Rtl

--connections

--order theta, pi, rho, chi, iota
theta_in<=state_in;
rho_w_in<=theta_out;
iota_in<=rho_w_out;
chi_in <= iota_out;
rho_e_in <= chi_out;
state_out<=rho_e_out;

--theta

--parity

i0101: for x in 0 to 3 generate
    i0102: for i in 0 to 31 generate
        p(x)(i)<=theta_in(0)(x)(i) xor theta_in(1)(x)(i) xor theta_in(2)(x)(i);
    end generate;
end generate;

i0111: for x in 0 to 3 generate
    i0112: for i in 0 to 31 generate
        e(x)(i)<=p((x-1) mod 4)((i-5) mod 32) xor p((x-1)mod 4)((i-14) mod 32);
    end generate;
end generate;

i0200: for y in 0 to 2 generate
    i0201: for x in 0 to 3 generate
        i0202: for i in 0 to 31 generate
            theta_out(y)(x)(i) <= theta_in(y)(x)(i) xor e(x)(i);
        end generate;
    end generate;
end generate;

-- rho west
i3001: for x in 0 to 3 generate
    i3002: for i in 0 to 31 generate
        rho_w_out(0)(x)(i) <= rho_w_in(0)(x)(i);
    end generate;
end generate;

i3012: for x in 0 to 3 generate
    i3013: for i in 0 to 31 generate
        rho_w_out(1)(x)(i) <= rho_w_in(1)((x-1)mod 4)(i);
    end generate;
end generate;

i3112: for x in 0 to 3 generate
    i3113: for i in 0 to 31 generate
        rho_w_out(2)(x)(i) <= rho_w_in(2)(x)((i-11) mod 32);
    end generate;
end generate;

-- iota
i4001: for x in 1 to 3 generate
    i4002: for i in 0 to 31 generate
        iota_out(0)(x)(i) <= iota_in(0)(x)(i);
    end generate;
end generate;

i4003: for i in 0 to 31 generate
    iota_out(0)(0)(i) <= iota_in(0)(0)(i) xor rc(i);
end generate;

i4011: for x in 0 to 3 generate
    i4012: for i in 0 to 31 generate
        iota_out(1)(x)(i) <= iota_in(1)(x)(i);
    end generate;
end generate;

i4021: for x in 0 to 3 generate
    i4022: for i in 0 to 31 generate
        iota_out(2)(x)(i) <= iota_in(2)(x)(i);
    end generate;
end generate;

-- chi

-- b0
i5001: for x in 0 to 3 generate
    i5002: for i in 0 to 31 generate
        chi_out(0)(x)(i) <= chi_in(0)(x)(i) xor (not(chi_in(1)(x)(i)) and chi_in(2)(x)(i));
    end generate;
end generate;
-- b1
i5011: for x in 0 to 3 generate
    i5012: for i in 0 to 31 generate
        chi_out(1)(x)(i) <= chi_in(1)(x)(i) xor (not(chi_in(2)(x)(i)) and chi_in(0)(x)(i));
    end generate;
end generate;
-- b2
i5021: for x in 0 to 3 generate
    i5022: for i in 0 to 31 generate
        chi_out(2)(x)(i) <= chi_in(2)(x)(i) xor (not(chi_in(0)(x)(i)) and chi_in(1)(x)(i));
    end generate;
end generate;


-- rho east
i6001: for x in 0 to 3 generate
    i6002: for i in 0 to 31 generate
        rho_e_out(0)(x)(i) <= rho_e_in(0)(x)(i);
    end generate;
end generate;

i6012: for x in 0 to 3 generate
    i6013: for i in 0 to 31 generate
        rho_e_out(1)(x)(i) <= rho_e_in(1)(x)((i-1)mod 32);
    end generate;
end generate;

i6112: for x in 0 to 3 generate
    i6113: for i in 0 to 31 generate
        rho_e_out(2)(x)(i) <= rho_e_in(2)((x-2) mod 4)((i-8) mod 32);
    end generate;
end generate;

end rtl;
