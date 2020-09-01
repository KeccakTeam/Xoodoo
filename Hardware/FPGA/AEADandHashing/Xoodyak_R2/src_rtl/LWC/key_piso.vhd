--------------------------------------------------------------------------------
--! @file       KEY_PISO.vhd
--! @brief      Width converter for NIST LWC API
--!
--! @author     Michael Tempelmeier
--! @copyright  Copyright (c) 2019 Chair of Security in Information Technology     
--!             ECE Department, Technical University of Munich, GERMANY

--! @license    This project is released under the GNU Public License.          
--!             The license and distribution terms for this file may be         
--!             found in the file LICENSE in this distribution or at            
--!             http://www.gnu.org/licenses/gpl-3.0.txt                         
--! @note       This is publicly available encryption source code that falls    
--!             under the License Exception TSU (Technology and software-       
--!             unrestricted)                                                  
--------------------------------------------------------------------------------
--! Description
--! This is a simplified version of the data_piso
--! 
--! TODO: Optimize t_state => t_state_16 and t_state_8
--! TODO: Change mux type => std_logic_vector => integer range 0 to 3, range 0 to 1
--! 
--! 
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use work.design_pkg.all;
use work.NIST_LWAPI_pkg.all;

entity KEY_PISO is 
    port(

            clk               : in std_logic;
            rst               : in std_logic;

            data_s             : out STD_LOGIC_VECTOR(CCSW-1 downto 0);
            data_valid_s       : out STD_LOGIC;
            data_ready_s       : in  STD_LOGIC;

            data_p             : in  STD_LOGIC_VECTOR(31 downto 0);
            data_valid_p       : in  STD_LOGIC;
            data_ready_p       : out STD_LOGIC

      );

end entity KEY_PISO;

architecture behavioral of KEY_PISO is

    type t_state is (LD_1, LD_2, LD_3, LD_4); 
    signal nx_state, state : t_state;
    signal mux : STD_LOGIC_VECTOR(3     -1 downto 0);

-- All key sizes are multiples of 32 bits. So our input size (data_size_p) is always 4 (bytes)
-- and we do not need to deal with partial input words.


begin

    assert (CCSW = 8) OR (CCSW = 16) or (CCSW=32) report "This module only supports CCSW={8,16,32}!" severity failure;

CCSW8_16: if CCSW /= 32 generate
    process (clk)
    begin
        if rising_edge(clk) then
            if(rst='1')  then
                state <= LD_1;
            else
                state <= nx_state;
            end if;
        end if;
    end process;
end generate CCSW8_16;

CCSW8: if CCSW = 8 generate

    process (state, data_valid_p, data_ready_s)
    begin
        case state is
            when LD_1 =>
                mux <= "001";
                if (data_ready_s = '1' and data_valid_p = '1') then 
                    nx_state <= LD_2;
                else
                    nx_state <= LD_1;
                end if;

           when LD_2 =>
               mux <= "010";
               if (data_ready_s = '1' and data_valid_p = '1') then 
                   nx_state <= LD_3;
               else
                   nx_state <= LD_2;
               end if;

           when LD_3 =>
               mux <= "011";
               if (data_ready_s = '1' and data_valid_p = '1') then 
                   nx_state <= LD_4;
               else
                   nx_state <= LD_3;
               end if;

           when LD_4 =>
               mux <= "100";
               if (data_ready_s = '1' and data_valid_p = '1') then
                   nx_state <= LD_1;
               else
                   nx_state <= LD_4;
               end if;

        end case;
    end process;

   -- controll signals are not set in the FSM to avoid circular dependency
   -- data_valid_* should not depend on data_ready_* and vice versa

   data_valid_s <= data_valid_p;
   data_ready_p <= data_ready_s when ("100" = mux) else '0'; -- if last word
   data_s       <= data_p (31 downto 24) when (mux = "001") else
                   data_p (23 downto 16) when (mux = "010") else
                   data_p (15 downto  8) when (mux = "011") else
                   data_p ( 7 downto  0);


end generate CCSW8;

CCSW16: if CCSW = 16 generate

    process (state, data_valid_p, data_ready_s)
    begin
        case state is
            when LD_1 =>
                mux <= "001";
                if (data_ready_s = '1' and data_valid_p = '1') then 
                    nx_state <= LD_2;
                else
                    nx_state <= LD_1;
                end if;

           when LD_2 =>
               mux <= "010";
               if (data_ready_s = '1' and data_valid_p = '1') then 
                    nx_state <= LD_1;
               else
                   nx_state <= LD_2;
               end if;

           when others =>
               nx_state <= state;
               mux <= "---";
               report "FSM error!" severity failure;

        end case;
    end process;

   -- controll signals are not set in the FSM to avoid circular dependency
   -- data_valid_* should not depend on data_ready_* and vice versa

   data_ready_p <= data_ready_s  when  (state = LD_2) else '0'; -- if last word
   data_valid_s <= data_valid_p;

   data_s       <= data_p (31 downto 16) when (mux = "001") else
                   data_p (15 downto  0);



end generate CCSW16;

CCSW32: if CCSW = 32 generate

    data_s <= data_p;
    data_valid_s <= data_valid_p;
    data_ready_p <= data_ready_s;

end generate CCSW32;

end behavioral;
