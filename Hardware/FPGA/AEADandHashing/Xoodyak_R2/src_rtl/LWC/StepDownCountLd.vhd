--------------------------------------------------------------------------------
--! @file       StepDownCountLd.vhd
--! @brief      n-bit step down counter with load
--! @author     Panasayya Yalla
--! @copyright  Copyright (c) 2016 Cryptographic Engineering Research Group
--!             ECE Department, George Mason University Fairfax, VA, U.S.A.
--!             All rights Reserved.
--! @license    This project is released under the GNU Public License.          
--!             The license and distribution terms for this file may be         
--!             found in the file LICENSE in this distribution or at            
--!             http://www.gnu.org/licenses/gpl-3.0.txt                         
--! @note       This is publicly available encryption source code that falls    
--!             under the License Exception TSU (Technology and software-       
--!             unrestricted)                                                  
--------------------------------------------------------------------------------
--! Description
--! N (Integer)  : Generic value to set the N
--! limit[N-1:0] : Final value of the counter
--! step         : Counter decrement value
--! clk          : Clock
--! ena          : Enable
--! len          : Load Enable
--! load[N-1:0]  : Load value
--! count[N-1:0] : Counter output
--------------------------------------------------------------------------------

library ieee;
use IEEE.STD_LOGIC_1164.all; 
use IEEE.NUMERIC_STD.ALL;


entity StepDownCountLd is 
    generic (   N       : integer;-- N of counter 
                step    : integer  --step value 
            );
    port    (
                clk         :in  std_logic;
                len         :in  std_logic;
                ena         :in  std_logic;
                load        :in  std_logic_vector(N-1 downto 0);
                count       :out std_logic_vector(N-1 downto 0)
            );
end StepDownCountLd;

architecture StepDownCountLd of StepDownCountLd is 
    signal qtemp:std_logic_vector(N-1 downto 0);
begin

    process(clk)
    begin
        if (clk'event and clk='1') then 
            if (len='1') then 
                qtemp<= load;
            elsif (ena  = '1') then
                qtemp <=std_logic_vector(unsigned(qtemp) - step);
            end if;
        end if;
    end process;
    count <= qtemp;
    
end StepDownCountLd;
