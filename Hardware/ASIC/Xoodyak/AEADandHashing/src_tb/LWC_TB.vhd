-------------------------------------------------------------------------------
--! @file       LWC_TB.vhd
--! @brief      Testbench based on the GMU CAESAR project.
--! @project    CAESAR Candidate Evaluation
--! @author     Ekawat (ice) Homsirikamol
--! @copyright  Copyright (c) 2015 Cryptographic Engineering Research Group
--!             ECE Department, George Mason University Fairfax, VA, U.S.A.
--!             All rights Reserved.
--! @version    1.0b1
--! @license    This project is released under the GNU Public License.
--!             The license and distribution terms for this file may be
--!             found in the file LICENSE in this distribution or at
--!             http://www.gnu.org/licenses/gpl-3.0.txt
--! @note       This is publicly available encryption source code that falls
--!             under the License Exception TSU (Technology and software-
--!             unrestricted)
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.ALL;
use ieee.numeric_std.all;
use ieee.std_logic_textio.all;
use work.std_logic_1164_additions.to_hstring; --needed, before VHDL-2008
use work.design_pkg.active_rst_p;
use work.NIST_LWAPI_pkg.all;

library std;
use std.textio.all;

entity LWC_TB IS
    generic (
        --! Test parameters
        G_STOP_AT_FAULT     : boolean := True;
        G_TEST_MODE         : integer := 0;
        G_TEST_IPSTALL      : integer := 3;
        G_TEST_ISSTALL      : integer := 3;
        G_TEST_OSTALL       : integer := 4;
        G_LOG2_FIFODEPTH    : integer := 8;
        G_PERIOD            : time    := 10 ns;
        G_FNAME_PDI         : string  := "../KAT/v1/pdi.txt";
        G_FNAME_SDI         : string  := "../KAT/v1/sdi.txt";
        G_FNAME_DO          : string  := "../KAT/v1/do.txt";
        G_FNAME_LOG         : string  := "xoodyak_log.txt";
        G_FNAME_RESULT      : string  := "xoodyak_result.txt"
    );
end LWC_TB;

architecture behavior of LWC_TB is

    --! bus width. 
    constant G_PWIDTH           : integer := W;
    constant G_SWIDTH           : integer := SW;
    -- for automated/scripted testing override:
    --    W and SW in work.NIST_LWAPI_pkg
    --    CCW and CCSW in work.design_pkg

    --! =================== --
    --! SIGNALS DECLARATION --
    --! =================== --

    --! simulation signals (used by ATHENa script, ignore if not used)
    signal simulation_fails     : std_logic := '0';
    signal stop_clock           : boolean   := False;

    --! error check signal
    signal global_stop          : std_logic := '1';

    --! globals
    signal clk                  : std_logic := '0';
    signal io_clk               : std_logic := '0';
    signal rst                  : std_logic := '0';

    --! pdi
    signal fpdi_din             : std_logic_vector(G_PWIDTH-1 downto 0)
        := (others=>'0');
    signal fpdi_din_valid       : std_logic := '0';
    signal fpdi_din_ready       : std_logic;
    signal fpdi_dout            : std_logic_vector(G_PWIDTH-1 downto 0);
    signal fpdi_dout_valid      : std_logic;
    signal fpdi_dout_ready      : std_logic;
    signal pdi_delayed          : std_logic_vector(G_PWIDTH-1 downto 0);
    signal pdi_valid            : std_logic;
    signal pdi_valid_selected   : std_logic;
    signal pdi_ready            : std_logic;

    --! sdi
    signal fsdi_din             : std_logic_vector(G_SWIDTH-1 downto 0)
        := (others=>'0');
    signal fsdi_din_valid       : std_logic := '0';
    signal fsdi_din_ready       : std_logic;
    signal fsdi_dout            : std_logic_vector(G_SWIDTH-1 downto 0);
    signal fsdi_dout_valid      : std_logic;
    signal fsdi_dout_ready      : std_logic;
    signal sdi_delayed          : std_logic_vector(G_SWIDTH-1 downto 0);
    signal sdi_valid            : std_logic;
    signal sdi_valid_selected   : std_logic;
    signal sdi_ready            : std_logic;

    --! do
    signal do                   : std_logic_vector(G_PWIDTH-1 downto 0);
    signal do_valid             : std_logic;
    signal do_last              : std_logic;
    signal do_ready             : std_logic;
    signal do_ready_selected    : std_logic;
    signal fdo_din_ready        : std_logic;
    signal fdo_din_valid        : std_logic;
    signal fdo_dout             : std_logic_vector(G_PWIDTH-1 downto 0);
    signal fdo_dout_valid       : std_logic;
    signal fdo_dout_ready       : std_logic := '0';

    --! Verification signals
    signal stall_pdi_valid      : std_logic := '0';
    signal stall_sdi_valid      : std_logic := '0';
    signal stall_do_full        : std_logic := '0';

    ------------- clock constant ------------------
    constant clk_period         : time := G_PERIOD;
    constant io_clk_period      : time := clk_period;
    ----------- end of clock constant -------------

    ------------- string constant ------------------
    --! constant
    constant cons_tb            : string(1 to 6) := "# TB :";
    constant cons_ins           : string(1 to 6) := "INS = ";
    constant cons_hdr           : string(1 to 6) := "HDR = ";
    constant cons_dat           : string(1 to 6) := "DAT = ";
    constant cons_stt           : string(1 to 6) := "STT = ";

    --! Shared constant
    constant cons_eof           : string(1 to 6) := "###EOF";
    ----------- end of string constant -------------

    ------------- debug constant ------------------
    constant debug_input        : boolean := False;
    constant debug_output       : boolean := False;
    ----------- end of clock constant -------------

    -------custom addon- need to be removed---
    signal tv_count:integer:=0;

    -- ================= --
    -- FILES DECLARATION --
    -- ================= --

    --------------- input / output files -------------------
    file pdi_file       : text open read_mode  is G_FNAME_PDI;
    file sdi_file       : text open read_mode  is G_FNAME_SDI;
    file do_file        : text open read_mode  is G_FNAME_DO;

    file log_file       : text open write_mode is G_FNAME_LOG;
    file result_file    : text open write_mode is G_FNAME_RESULT;
    ------------- end of input files --------------------
    signal TestVector : integer;
begin

    genClk: process
    begin
        if (not stop_clock and global_stop = '1') then
            clk <= '1';
            wait for clk_period/2;
            clk <= '0';
            wait for clk_period/2;
        else
            wait;
        end if;
    end process genClk;

    genIOclk: process
    begin
        if ((not stop_clock) and (global_stop = '1')) then
            io_clk <= '1';
            wait for io_clk_period/2;
            io_clk <= '0';
            wait for io_clk_period/2;
        else
            wait;
        end if;
    end process genIOclk;

    --! ============ --
    --! PORT MAPPING --
    --! ============ --
    genPDIfifo: entity work.fwft_fifo(structure)
    generic map (
        G_W          => G_PWIDTH,
        G_LOG2DEPTH  => G_LOG2_FIFODEPTH)
    port map (
        clk          =>  io_clk,
        rst          =>  rst,
        din          =>  fpdi_din,
        din_valid    =>  fpdi_din_valid,
        din_ready    =>  fpdi_din_ready,
        dout         =>  fpdi_dout,
        dout_valid   =>  fpdi_dout_valid,
        dout_ready   =>  fpdi_dout_ready
    );

    fpdi_dout_ready     <= '0' when stall_pdi_valid = '1' else pdi_ready;
    pdi_valid_selected  <= '0' when stall_pdi_valid = '1' else fpdi_dout_valid;
    pdi_valid           <= pdi_valid_selected after 1/4*clk_period;
    pdi_delayed         <= fpdi_dout after 1/4*clk_period;

    genSDIfifo: entity work.fwft_fifo(structure)
    generic map (
        G_W          => G_SWIDTH,
        G_LOG2DEPTH  => G_LOG2_FIFODEPTH)
    port map (
        clk          =>  io_clk,
        rst          =>  rst,
        din          =>  fsdi_din,
        din_valid    =>  fsdi_din_valid,
        din_ready    =>  fsdi_din_ready,
        dout         =>  fsdi_dout,
        dout_valid   =>  fsdi_dout_valid,
        dout_ready   =>  fsdi_dout_ready
    );

    fsdi_dout_ready     <= '0' when stall_sdi_valid = '1' else sdi_ready;
    sdi_valid_selected  <= '0' when stall_sdi_valid = '1' else fsdi_dout_valid;
    sdi_valid           <= sdi_valid_selected after 1/4*clk_period;
    sdi_delayed         <= fsdi_dout after 1/4*clk_period;

    genDOfifo: entity work.fwft_fifo(structure)
    generic map (
        G_W          => G_PWIDTH,
        G_LOG2DEPTH  => G_LOG2_FIFODEPTH)
    port map (
        clk          =>  io_clk,
        rst          =>  rst,
        din          =>  do,
        din_valid    =>  fdo_din_valid,
        din_ready    =>  fdo_din_ready,
        dout         =>  fdo_dout,
        dout_valid   =>  fdo_dout_valid,
        dout_ready   =>  fdo_dout_ready
    );

    fdo_din_valid       <= '0' when stall_do_full = '1' else do_valid;
    do_ready_selected   <= '0' when stall_do_full = '1' else fdo_din_ready;
    do_ready            <= do_ready_selected after 1/4*clk_period;

    uut:  entity work.LWC(structure)
    port map (
        rst          => rst,
        clk          => clk,
        pdi_data     => pdi_delayed,
        pdi_ready    => pdi_ready,
        pdi_valid    => pdi_valid,
        sdi_data     => sdi_delayed,
        sdi_ready    => sdi_ready,
        sdi_valid    => sdi_valid,
        do_data      => do,
        do_valid     => do_valid,
        do_last      => do_last,
        do_ready     => do_ready
    );
    --! =================== --
    --! END OF PORT MAPPING --
    --! =================== --


    --! =======================================================================
    --! ==================== DATA POPULATION FOR PUBLIC DATA ==================
    tb_read_pdi : process
        variable line_data      : line;
        variable word_block     : std_logic_vector(G_PWIDTH-1 downto 0)
            := (others=>'0');
        variable read_result    : boolean;
        variable loop_enable    : std_logic   := '1';
        variable temp_read      : string(1 to 6);
        variable valid_line     : boolean     := True;
    begin
        rst <= active_rst_p;               wait for 5*clk_period;
        rst <= not(active_rst_p);               wait for clk_period;

        --! read header
        while ( not endfile (pdi_file)) and ( loop_enable = '1' ) loop
            if endfile (pdi_file) then
                loop_enable := '0';
            end if;

            readline(pdi_file, line_data);
            read(line_data, temp_read, read_result);
            if (temp_read = cons_ins) then
                loop_enable := '0';
            end if;
        end loop;

        --! do operations in the falling edge of the io_clk
        wait for io_clk_period/2;

        while not endfile ( pdi_file ) loop
            --! if the fifo is full, wait ...
            fpdi_din_valid <= '1';
            if ( fpdi_din_ready = '0' ) then
                fpdi_din_valid <= '0';
                wait until  fpdi_din_ready <= '1';
                wait for    io_clk_period/2; --! write in the rising edge
                fpdi_din_valid <= '1';
            end if;

            hread( line_data, word_block, read_result );
            while (((read_result = False) or (valid_line = False))
                and (not endfile( pdi_file )))
            loop
                readline(pdi_file, line_data);
                read(line_data, temp_read, read_result);    --! read line header
                if ( temp_read = cons_ins or temp_read = cons_hdr
                    or temp_read = cons_dat)
                then
                    valid_line := True;
                    fpdi_din_valid  <= '1';
                else
                    valid_line := False;
                    fpdi_din_valid  <= '0';
                end if;
                hread( line_data, word_block, read_result ); --! read data
            end loop;
            fpdi_din <= word_block;
               wait for io_clk_period;
        end loop;
        tv_count<= tv_count+1;
        fpdi_din_valid <= '0';
        wait;
    end process;
    --! =======================================================================
    --! ==================== DATA POPULATION FOR SECRET DATA ==================
    tb_read_sdi : process
        variable line_data      : line;
        variable word_block     : std_logic_vector(G_SWIDTH-1 downto 0)
            := (others=>'0');
        variable read_result    : boolean;
        variable loop_enable    : std_logic := '1';
        variable temp_read      : string(1 to 6);
        variable valid_line     : boolean := True;
    begin
        --! Wait until reset is done
        wait for 7*clk_period;

        --! read header
        while (not endfile (sdi_file)) and (loop_enable = '1') loop
            if endfile (sdi_file) then
                loop_enable := '0';
            end if;

            readline(sdi_file, line_data);
            read(line_data, temp_read, read_result);
            if (temp_read = cons_ins) then
                loop_enable := '0';
            end if;
        end loop;

        --! do operations in the falling edge of the io_clk
        wait for io_clk_period/2;

        while not endfile ( sdi_file ) loop
            --! if the fifo is full, wait ...
            fsdi_din_valid <= '1';
            if ( fsdi_din_ready = '0' ) then
                fsdi_din_valid <= '0';
                wait until  fsdi_din_ready <= '1';
                wait for    io_clk_period/2; --! write in the rising edge
                fsdi_din_valid <= '1';
            end if;

            hread(line_data, word_block, read_result);
            while (((read_result = False) or (valid_line = False))
                and (not endfile(sdi_file)))
            loop
                readline(sdi_file, line_data);
                read(line_data, temp_read, read_result);   --! read line header
                if (temp_read = cons_ins or temp_read = cons_hdr
                    or temp_read = cons_dat)
                then
                    valid_line := True;
                    fsdi_din_valid  <= '1';
                else
                    valid_line := False;
                    fsdi_din_valid  <= '0';
                end if;
                hread( line_data, word_block, read_result );    --! read data
            end loop;
            fsdi_din <= word_block;
            wait for io_clk_period;
        end loop;
        fsdi_din_valid <= '0';
        wait;
    end process;
    --! =======================================================================


    --! =======================================================================
    --! =================== DATA VERIFICATION =================================
    tb_verifydata : process
        variable line_no        : integer := 0;
        variable line_data      : line;
        variable logMsg         : line;
        variable tb_block       : std_logic_vector(20      -1 downto 0);
        variable word_block     : std_logic_vector(G_PWIDTH-1 downto 0)
            := (others=>'0');
        variable read_result    : boolean;
        variable temp_read      : string(1 to 6);
        variable valid_line     : boolean := True;
        variable word_count     : integer := 1;
        variable word_pass      : integer := 1;
        variable instr_encoding : boolean := False;
        variable force_exit     : boolean := False;
        variable msgid          : integer;
        variable keyid          : integer;
        variable isEncrypt      : boolean := False;
        variable opcode         : std_logic_vector(3 downto 0);
    begin
        wait for 6*clk_period;

        while (not endfile (do_file) and valid_line and (not force_exit)) loop
            --! Keep reading new line until a valid line is found
            hread( line_data, word_block, read_result );
            while ((read_result = False or valid_line = False)
                  and (not endfile(do_file)))
            loop
                readline(do_file, line_data);
                line_no := line_no + 1;
                read(line_data, temp_read, read_result); --! read line header
                if (temp_read = cons_hdr
                    or temp_read = cons_dat
                    or temp_read = cons_stt)
                then
                    valid_line := True;
                    word_count := 1;
                else
                    valid_line := False;
                    if (temp_read = cons_tb) then
                        instr_encoding := True;
                    end if;
                end if;

                if (temp_read = cons_eof) then
                    force_exit := True;
                end if;

                if (instr_encoding = True) then
                    hread(line_data, tb_block, read_result); --! read data
                    instr_encoding := False;
                    read_result    := False;
                    opcode := tb_block(19 downto 16);
                    keyid  := to_integer(unsigned(tb_block(15 downto 8)));
                    msgid  := to_integer(unsigned(tb_block(7  downto 0)));
                    TestVector<= to_integer(unsigned(tb_block(7  downto 0)));
                    isEncrypt := False;
                    if ((opcode = INST_DEC or opcode = INST_ENC)
                        or (opcode = INST_SUCCESS or opcode = INST_FAILURE))
                    then
                        write(logMsg, string'("[Log] == Verifying msg ID #")
                            & integer'image(msgid)
                            & string'(" with key ID #") & integer'image(keyid));
                        if (opcode = INST_ENC) then
                            isEncrypt := True;
                            write(logMsg, string'(" for ENC"));
                        else
                            write(logMsg, string'(" for DEC"));
                        end if;
                        writeline(log_file,logMsg);
                    end if;

                    report "---------Started verifying message number "
                        & integer'image(msgid) & " at "
                        & time'image(now) severity note;
                else
                    hread(line_data, word_block, read_result); --! read data
                end if;
            end loop;

            --! if the core is slow in outputting the digested message, wait ...
            if ( valid_line ) then
                fdo_dout_ready <= '1';
                if ( fdo_dout_valid = '0') then
                    fdo_dout_ready <= '0';
                    wait until fdo_dout_valid = '1';
                    wait for io_clk_period/2;
                    fdo_dout_ready <= '1';
                end if;

                word_pass := 1;
                for i in G_PWIDTH-1 downto 0 loop
                    if  fdo_dout(i) /= word_block(i)
                        and word_block(i) /= 'X'
                    then
                        word_pass := 0;
                    end if;
                end loop;
                if word_pass = 0 then
                    simulation_fails <= '1';
                    write(logMsg, string'("[Log] Msg ID #")
                        & integer'image(msgid)
                        & string'(" fails at line #") & integer'image(line_no)
                        & string'(" word #") & integer'image(word_count));
                    writeline(log_file,logMsg);
                    write(logMsg, string'("[Log]     Expected: ")
                        & to_hstring(word_block)
                        & string'(" Received: ") & to_hstring(fdo_dout));
                    writeline(log_file,logMsg);

                    --! Stop the simulation right away when an error is detected
                    report "---------Data line #"  & integer'image(line_no)
                        & " Msg ID #" & integer'image(msgid)
                        & " Key ID #" & integer'image(keyid)
                        & " Word #" & integer'image(word_count)
                        & " at " & time'image(now) & " FAILS T_T --------"
                        severity error;
                    report "Expected: " & to_hstring(word_block)
                        & " Actual: " & to_hstring(fdo_dout) severity error;
                    write(result_file, "fail");
                    if (G_STOP_AT_FAULT = True) then
                        force_exit := True;
                    else
                        if isEncrypt = False then
                            report "---------Skip to the next instruction"
                                & " at " & time'image(now) severity error;
                            write(logMsg,
                                string'("[Log]  ...skips to next message ID"));
                            writeline(log_file, logMsg);
                        end if;
                    end if;
                else
                    write(logMsg, string'("[Log]     Expected: ")
                        & to_hstring(word_block)
                        & string'(" Received: ") & to_hstring(fdo_dout)
                        & string'(" Matched!"));
                    writeline(log_file,logMsg);
                end if;

                wait for io_clk_period;
                word_count := word_count + 1;
            end if;
        end loop;

        fdo_dout_ready <= '0';
        wait for io_clk_period;

        if (simulation_fails = '1') then
            report "FAIL (1): SIMULATION FINISHED || Input/Output files :: T_T"
                & G_FNAME_PDI & "/" & G_FNAME_SDI
                & "/" & G_FNAME_DO severity error;
            write(result_file, "1");
        else
            report "PASS (0): SIMULATION FINISHED || Input/Output files :: ^0^"
                & G_FNAME_PDI & "/" & G_FNAME_SDI
                & "/" & G_FNAME_DO severity error;
            write(result_file, "0");
        end if;
        write(logMsg, string'("[Log] Done"));
        writeline(log_file,logMsg);
        stop_clock <= True;
        wait;
    end process;
    --! =======================================================================


    --! =======================================================================
    --! =================== Test MODE =========================================
    genInputStall1 : process
    begin
        if G_TEST_MODE = 1 or G_TEST_MODE = 2 then
            wait until rising_edge(io_clk);
            wait for 1/4*io_clk_period;
            if (pdi_ready = '0') then
                wait until falling_edge(io_clk) and pdi_ready = '1';
            end if;
            if (pdi_valid = '0') then
                wait until falling_edge(io_clk) and pdi_valid = '1';
            end if;
            wait for io_clk_period;
            stall_pdi_valid <= '1';
            wait for io_clk_period*G_TEST_IPSTALL;
            stall_pdi_valid <= '0';
        else
            wait;
        end if;
    end process;

    genInputStall2 : process
    begin
        if G_TEST_MODE = 1 or G_TEST_MODE = 2 then
            wait until rising_edge(io_clk);
            wait for 1/4*io_clk_period;
            if (sdi_ready = '0') then
                wait until falling_edge(io_clk) and sdi_ready = '1';
            end if;
            if (sdi_valid = '0') then
                wait until falling_edge(io_clk) and sdi_valid = '1';
            end if;
            wait for io_clk_period;
            stall_sdi_valid <= '1';
            wait for io_clk_period*G_TEST_ISSTALL;
            stall_sdi_valid <= '0';
        else
            wait;
        end if;
    end process;

    genOutputStall : process
    begin
        if G_TEST_MODE = 1 or G_TEST_MODE = 3 then
            wait until rising_edge(io_clk);
            wait for 1/4*io_clk_period;
            if (do_ready = '0') then
                wait until falling_edge(io_clk) and do_ready = '1';
            end if;
            if (do_valid = '0') then
                wait until falling_edge(io_clk) and do_valid = '1';
            end if;
            wait for io_clk_period;
            stall_do_full <= '1';
            wait for io_clk_period*G_TEST_OSTALL;
            stall_do_full <= '0';
        else
            wait;
        end if;
    end process;
end;
