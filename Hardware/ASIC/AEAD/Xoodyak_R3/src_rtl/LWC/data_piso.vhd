--------------------------------------------------------------------------------
--! @file       data_piso.vhd
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
--! TODO: Change mux type => std_logic_vector => integer range 1 to 4, range 1 to 2
--! 
--! 
--! 
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use work.design_pkg.all;
use work.NIST_LWAPI_pkg.all;


entity data_piso is 
    port(

            clk               : in std_logic;
            rst               : in std_logic;

            data_size_p       :  in STD_LOGIC_VECTOR(3     -1 downto 0);
            data_size_s       : out STD_LOGIC_VECTOR(3     -1 downto 0);



            data_s             : out STD_LOGIC_VECTOR(CCW-1 downto 0);
            data_valid_s       : out STD_LOGIC;
            data_ready_s       : in  STD_LOGIC;

            data_p             : in  STD_LOGIC_VECTOR(31 downto 0);
            data_valid_p       : in  STD_LOGIC;
            data_ready_p       : out STD_LOGIC;

            valid_bytes_p      :  in STD_LOGIC_VECTOR(4-1 downto 0);
            valid_bytes_s      : out STD_LOGIC_VECTOR(CCWdiv8-1 downto 0);
            pad_loc_p          :  in STD_LOGIC_VECTOR(4-1 downto 0);
            pad_loc_s          : out STD_LOGIC_VECTOR(CCWdiv8-1 downto 0);

            eoi_p              :  in std_logic;
            eoi_s              : out std_logic;

            eot_p              :  in std_logic;
            eot_s              : out std_logic
      );

end entity data_piso;

architecture behavioral of data_piso is

    type t_state is (LD_1, LD_2, LD_3, LD_4); 
    signal nx_state, state : t_state;
    signal mux : STD_LOGIC_VECTOR(3     -1 downto 0);
    signal last: std_logic;



begin

    assert (CCW = 32) report "This module only supports CCW=32!" severity failure;

CCW32: if CCW = 32 generate

    data_s <= data_p;
    data_valid_s <= data_valid_p;
    data_ready_p <= data_ready_s;

    valid_bytes_s <= valid_bytes_p;
    pad_loc_s <= pad_loc_p;

    eoi_s           <= eoi_p;
    eot_s           <= eot_p;

    data_size_s <= data_size_p;

end generate CCW32;

end behavioral;
