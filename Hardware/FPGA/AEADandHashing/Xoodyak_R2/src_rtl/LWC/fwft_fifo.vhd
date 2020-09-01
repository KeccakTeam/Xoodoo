--------------------------------------------------------------------------------
--! @file       fwft_fifo.vhd
--! @brief      Simple First-In-First_Out
--!
--! @author     Patrick Karl <patrick.karl@tum.de>
--! @copyright  Copyright (c) 2019 Chair of Security in Information Technology
--!             ECE Department, Technical University of Munich, GERMANY
--!
--! @license    This project is released under the GNU Public License.
--!             The license and distribution terms for this file may be
--!             found in the file LICENSE in this distribution or at
--!             http://www.gnu.org/licenses/gpl-3.0.txt
--!
--------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity fwft_fifo is
    generic(
        G_W         : integer; --! Width of I/O (bits)
        G_LOG2DEPTH : integer  --! LOG(2) of depth
    );
    port(
        clk         : in    std_logic;
        rst         : in    std_logic;

        -- Input Port
        din         : in    std_logic_vector(G_W - 1 downto 0);
        din_valid   : in    std_logic;
        din_ready   : out   std_logic;

        -- Output Port
        dout        : out   std_logic_vector(G_W - 1 downto 0);
        dout_valid  : out   std_logic;
        dout_ready  : in    std_logic

        --prog_full_th    : in    std_logic_vector(G_LOG2DEPTH - 1 downto 0);
        --prog_full       : out   std_logic
    );
end entity fwft_fifo;

architecture structure of fwft_fifo is

    -- FIFO depth in words
    constant DEPTH_C : integer := 2**G_LOG2DEPTH;

    -- Memory type and signal definition
    type mem_t is array (0 to DEPTH_C - 1) of std_logic_vector(G_W - 1 downto 0);
    signal mem_s            : mem_t;

    -- Internal handshake signals
    signal din_ready_s      : std_logic := '0';
    signal dout_valid_s     : std_logic := '0';

    -- Internal flags
    signal empty_s          : std_logic;
    signal full_s           : std_logic;
    signal wr_ptr_s         : integer range 0 to DEPTH_C - 1;
    signal rd_ptr_s         : integer range 0 to DEPTH_C - 1;

    -- Although we use an additional counter for generating din_ready and
    -- dout_valid, standalone synthesis shows less ressource usage.
    -- In addition to that, the entries counter can be used for
    -- generating additional full/empty flags and programmable full/empty flags.
    signal entries_s        : integer range 0 to DEPTH_C;

begin

    -- Output data
    dout        <= mem_s(rd_ptr_s);
    dout_valid  <= dout_valid_s;
    din_ready   <= din_ready_s;

    -- Set flags
    --prog_full       <= '1' when (entries_s >= to_integer(unsigned(prog_full_th))) else '0';
    full_s          <= '1' when (entries_s >= DEPTH_C)      else '0';
    empty_s         <= '1' when (entries_s <= 0)            else '0';
    din_ready_s     <= not full_s;
    dout_valid_s    <= not empty_s;


    -- Counting the numbers of entries, setting rd-/wr-pointers and
    -- writing the data into the memory.
    p_ptr : process(clk)
    begin
        if rising_edge(clk) then
            if (rst = '1') then
                wr_ptr_s    <= 0;
                rd_ptr_s    <= 0;
                entries_s   <= 0;
            else

                -- Increase entry counter if data is written but not read
                -- Decrease entry counter if data is read but not written
                if (din_valid = '1' and din_ready_s = '1'
                and (dout_valid_s = '0' or dout_ready = '0')) then
                    entries_s <= entries_s + 1;
                elsif ((din_valid = '0' or din_ready_s = '0')
                and dout_valid_s = '1' and dout_ready = '1') then
                    entries_s <= entries_s - 1;
                end if;

                -- Write into memory and increase write pointer
                if (din_valid = '1' and din_ready_s = '1') then
                    mem_s(wr_ptr_s) <= din;
                    if (wr_ptr_s >= DEPTH_C - 1) then
                        wr_ptr_s <= 0;
                    else
                        wr_ptr_s <= wr_ptr_s + 1;
                    end if;
                end if;

                -- Increase read pointer if data is read
                if (dout_valid_s = '1' and dout_ready = '1') then
                    if (rd_ptr_s >= DEPTH_C - 1) then
                        rd_ptr_s <= 0;
                    else
                        rd_ptr_s <= rd_ptr_s + 1;
                    end if;
                end if;

            end if;
        end if;
    end process p_ptr;

end architecture structure;
