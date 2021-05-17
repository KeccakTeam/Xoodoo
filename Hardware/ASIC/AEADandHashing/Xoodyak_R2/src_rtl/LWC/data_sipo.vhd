--------------------------------------------------------------------------------
--! @file       data_sipo.vhd
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
--! 
--! TODO: Optimize t_state => t_state_16 and t_state_8
--! 
--! 
--! 
--! 
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use work.design_pkg.all;
use work.NIST_LWAPI_pkg.all;

entity data_sipo is 
    port(

            clk               : in std_logic;
            rst               : in std_logic;

            end_of_input      : in STD_LOGIC;

            data_p             : out STD_LOGIC_VECTOR(31 downto 0);
            data_valid_p       : out STD_LOGIC;
            data_ready_p       : in  STD_LOGIC;

            data_s             : in  STD_LOGIC_VECTOR(CCW-1 downto 0);
            data_valid_s       : in  STD_LOGIC;
            data_ready_s       : out STD_LOGIC

      );

end entity data_sipo;

architecture behavioral of data_sipo is

    type t_state is (LD_1, LD_2, LD_3, LD_4); 
    signal nx_state, state : t_state;
    signal mux : integer range 1 to 4;
    signal reg : std_logic_vector (31 downto 8);


begin

    assert (CCW = 32) report "This module only supports CCW=32!" severity failure;

CCW32: if CCW = 32 generate --No PISO needed

    data_p       <= data_s;
    data_valid_p <= data_valid_s;
    data_ready_s <= data_ready_p;

end generate;

end behavioral;
