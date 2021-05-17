--------------------------------------------------------------------------------
--! @file       PostProcessor.vhd
--! @brief      Post-processor for NIST LWC API
--!
--! @author     Michael Tempelmeier
--! @copyright  Copyright (c) 2019 Chair of Security in Information Technology
--!             ECE Department, Technical University of Munich, GERMANY
--!
--! @author     Farnoud Farahmand
--! @copyright  Copyright (c) 2019 Cryptographic Engineering Research Group
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
--!
--!  bdo_type is not used at the moment.
--!  However, we encourage authors to provide it, as it helps to adapt the
--!  CryptoCore to different use cases. Additionally, it might get needed in a
--!  future version of the PostProcessor.
--!
--!  There is no penalty in terms of area, as this signal gets trimmed during
--!  synthesis.
--!
--!
--------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use work.NIST_LWAPI_pkg.all;
use work.design_pkg.all;

entity PostProcessor is

    Port (
            clk             : in  std_logic;
            rst             : in  std_logic;
            --! Crypto Core ====================================================
            bdo             : in  std_logic_vector(CCW-1 downto 0);
            bdo_valid       : in  std_logic;
            bdo_ready       : out std_logic;
            end_of_block    : in  std_logic;
            bdo_type        : in  std_logic_vector(3 downto 0); -- not used atm
            bdo_valid_bytes : in  std_logic_vector(CCWdiv8-1 downto 0);
            msg_auth        : in  std_logic;
            msg_auth_ready  : out std_logic;
            msg_auth_valid  : in  std_logic;
            ---! Header FIFO ===================================================
            cmd             : in  std_logic_vector(W-1 downto 0);
            cmd_valid       : in  std_logic;
            cmd_ready       : out std_logic;
            --! Data Output (do) ===============================================
            do_data         : out std_logic_vector(W-1 downto 0);
            do_valid        : out std_logic;
            do_last         : out std_logic;
            do_ready        : in  std_logic
    );

end PostProcessor;

architecture PostProcessor of PostProcessor is

    --Signals
    signal do_data_internal     : std_logic_vector(W-1 downto 0);
    signal do_valid_internal    : std_logic;
    signal bdo_cleared          : std_logic_vector(CCW-1 downto 0);
    signal len_SegLenCnt        : std_logic;
    signal en_SegLenCnt         : std_logic;
    signal dout_SegLenCnt       : std_logic_vector(15 downto 0);
    signal load_SegLenCnt       : std_logic_vector(15 downto 0);
    signal last_flit_of_segment : std_logic;


    --Registers
    signal decrypt, nx_decrypt  : std_logic;
    signal eot, nx_eot          : std_logic;

    --Aliases
    alias cmd_opcode            : std_logic_vector( 3 downto 0) is cmd(W-1 downto W-4);
    alias cmd_seg_length        : std_logic_vector((W/2)-1 downto 0) is cmd((W/2)-1 downto  0);

    --Constants
    constant HASHdiv8           : integer := HASH_VALUE_SIZE/8;
    constant TAGdiv8            : integer := TAG_SIZE/8;
    constant zero_data          : std_logic_vector(W-1 downto 0):=(others=>'0');

    --State types for different I/O sizes
    --! State for W=SW=32
    type t_state32 is (
                    S_INIT,S_HDR_HASH_VALUE, S_OUT_HASH_VALUE,
                    S_HDR_MSG, S_OUT_MSG, S_HDR_TAG, S_OUT_TAG, S_VER_TAG, 
                    S_STATUS_FAIL, S_STATUS_SUCCESS
                );

begin

    -- set unused bytes to zero
    bdo_cleared <= bdo and Byte_To_Bits_EXP(bdo_valid_bytes);

    -- make sure we do not output intermeadiate data
    do_valid <= do_valid_internal;
    do_data  <= do_data_internal when (do_valid_internal='1') else do_data_defaults;


    --! Segment Length Counter
    -- This counter can be saved, if we do not want to support multiple segments
    SegLen: entity work.StepDownCountLd(StepDownCountLd)
                generic map(
                        N       =>  16,
                        step    =>  Wdiv8
                            )
                port map
                        (
                        rst     => rst,
                        clk     =>  clk ,
                        len     =>  len_SegLenCnt,
                        load    =>  load_SegLenCnt,
                        ena     =>  en_SegLenCnt,
                        count   =>  dout_SegLenCnt
                    );

    last_flit_of_segment <= '1' when (to_integer(unsigned(dout_SegLenCnt))<= Wdiv8) else '0';

    --! Registers
    -- state register depends on W and is set in the corresponding if generate
    process (clk,rst)
    begin
        if (rst = active_rst_p) then
            eot     <= '0';
            decrypt <= '0';
        elsif rising_edge(clk) then
            eot     <= nx_eot;
            decrypt <= nx_decrypt;
        end if;
    end process;



   -- ====================================================================================================
   --! 32 bit specific FSM -------------------------------------------------------------------------------
   -- ====================================================================================================

FSM_32BIT: if (W=32) generate

    --! 32 Bit specific declarations
    alias do_data_internal_opcode   : std_logic_vector( 3 downto 0) is do_data_internal(31 downto 28);
    alias do_data_internal_flags    : std_logic_vector( 3 downto 0) is do_data_internal(27 downto 24);
    alias do_data_internal_reserved : std_logic_vector( 7 downto 0) is do_data_internal(23 downto 16);
    alias do_data_internal_length   : std_logic_vector(15 downto 0) is do_data_internal(15 downto  0);
    
    --sipo
    signal bdo_valid_p              : std_logic;
    signal bdo_ready_p              : std_logic;
    signal bdo_p                    : std_logic_vector(31 downto 0);

    signal nx_state, pr_state       : t_state32;

    begin

    load_SegLenCnt       <= cmd_seg_length;

    --! SIPO
    -- for ccw < W: a sipo is used for width conversion
    bdoSIPO: entity work.data_sipo(behavioral) port map
        (
            clk => clk,
            rst => rst,
            -- no need for conversion, as our last serial_in element is also the
            -- last parallel_out element
            end_of_input  => end_of_block,
            -- end_of_bock should only be evaluated if bdo_valid_p = '1'
            data_s       => bdo_cleared,
            data_valid_s => bdo_valid,
            data_ready_s => bdo_ready,

            data_p       => bdo_p,
            data_valid_p => bdo_valid_p,
            data_ready_p => bdo_ready_p
        );

    --! State register
    process (clk,rst)
    begin
        if (rst = active_rst_p) then
                pr_state <= S_INIT;
        elsif rising_edge(clk) then
                pr_state <= nx_state;
            end if;
    end process;


    --! Next state function
    process (pr_state, bdo_valid_p, do_ready, end_of_block, decrypt,
            cmd_valid, msg_auth_valid, msg_auth, last_flit_of_segment,
            cmd_opcode, eot)

    begin
        case pr_state is

            when S_INIT =>
                if (cmd_valid = '1') then
                    if (cmd_opcode = INST_HASH) then
                        nx_state <= S_HDR_HASH_VALUE;
                    else
                        nx_state <= S_HDR_MSG;
                    end if;
                else
                    nx_state <= S_INIT;
                end if;

           --Hash
           when S_HDR_HASH_VALUE =>
                if (do_ready = '1') then
                    nx_state <= S_OUT_HASH_VALUE;
                else
                    nx_state <= S_HDR_HASH_VALUE;
                end if;

           when S_OUT_HASH_VALUE =>
                if (bdo_valid_p = '1' and do_ready = '1' and end_of_block = '1') then
                    nx_state <= S_STATUS_SUCCESS;
                else
                    nx_state <= S_OUT_HASH_VALUE;
                end if;

            --MSG
            when S_HDR_MSG =>
                if (cmd_valid = '1' and do_ready = '1') then
                    if (cmd_seg_length = x"0000") then
                        if (decrypt='1') then
                            nx_state <= S_VER_TAG;
                        else
                            nx_state <= S_HDR_TAG;
                        end if;
                    else
                        nx_state <= S_OUT_MSG;
                    end if;
                else
                    nx_state <= S_HDR_MSG;
                end if;

            when S_OUT_MSG =>
                if (bdo_valid_p = '1' and do_ready = '1') then
                    -- This line is needed, if the input (and therefore) the
                    -- output is splitted in multiple segments.
                    if (last_flit_of_segment = '1') then
                    -- This line can be used instead, if there is only one 
                    -- input segment and there is no ciphertext expansion.
                    -- This saves us the output counter.
                    --if (end_of_block = '1') then
                        
                        -- this is the last segment
                        if (eot = '1') then
                            if (decrypt = '1') then
                                nx_state <= S_VER_TAG;
                            else
                                nx_state <= S_HDR_TAG;
                            end if;
                        else
                            -- this is not the last segment, we have multiple segments
                            nx_state <= S_HDR_MSG;
                        end if;
                    else
                        -- more output in current segment
                        nx_state <= S_OUT_MSG;
                    end if;
                else
                    nx_state <= S_OUT_MSG;
                end if;

            --TAG
            when S_HDR_TAG =>
                if (do_ready = '1') then
                    nx_state <= S_OUT_TAG;
                else
                    nx_state <= S_HDR_TAG;
                end if;

            when S_OUT_TAG =>
                if (bdo_valid_p = '1' and end_of_block='1' and do_ready='1') then
                    nx_state <= S_STATUS_SUCCESS;
                else
                    nx_state <= S_OUT_TAG;
                end if;

            when S_VER_TAG =>
                if (msg_auth_valid = '1') then
                    if (msg_auth = '1') then
                        nx_state <= S_STATUS_SUCCESS;
                    else
                        nx_state <= S_STATUS_FAIL;
                    end if;
                else
                    nx_state <= S_VER_TAG;
                end if;

            -- STATUS
            when S_STATUS_FAIL =>
                if (do_ready = '1') then
                    nx_state <= S_INIT;
                else
                    nx_state <= S_STATUS_FAIL;
                end if;

            when S_STATUS_SUCCESS =>
                if (do_ready = '1') then
                    nx_state <= S_INIT;
                else
                    nx_state <= S_STATUS_SUCCESS;
                end if;

            when others=>
                    nx_state <= pr_state;
        end case;
    end process;


    --! Output state function
    process(pr_state, bdo_valid_p, bdo_p, decrypt, eot, cmd, cmd_valid, do_ready)
    begin
            -- DEFAULT SIGNALS
            -- external interface
            do_last           <='0';
            do_data_internal  <= (others => '0');--(others => '-');
            do_valid_internal <='0';
            -- CryptoCore
            bdo_ready_p       <='0';
            msg_auth_ready    <='0';
            -- Header-FIFO
            cmd_ready         <='0';
            -- Segment counter
            len_SegLenCnt     <='0';
            en_SegLenCnt      <='0';
            -- Registers
            nx_decrypt        <= decrypt;
            nx_eot            <= eot;

        case pr_state is
            when S_INIT =>

                if (cmd_valid = '1') then
                    -- We reiceive either INST_HASH, or INST_ENC or INST_DEC
                    -- The LSB of INST_ENC and INST_DEC is '1' for Decryption
                    -- For Hash, this bit is '0', however we never evaluate
                    -- "decrypt" for Hash.
                    nx_decrypt <= cmd(28);
                end if;

                cmd_ready <= '1';

            --MSG
            when S_HDR_MSG =>
                cmd_ready          <= do_ready;
                len_SegLenCnt      <= do_ready and cmd_valid;
                do_valid_internal  <= cmd_valid;
                -- preserve EOT flag to support multi segment MSGs
                nx_eot             <= cmd(25);

                if (decrypt = '1') then
                    -- header is plaintext
                    do_data_internal_opcode   <= HDR_PT;

                    -- last: no TAG is sent after decryption.
                    -- If cmd(25) = '0' (EOT ='0') then we have multiple segments,
                    -- and this is not the last one.
                    do_data_internal_flags(0) <= '1' AND cmd(25);
                else
                    -- header is ciphertext
                    do_data_internal_opcode   <= HDR_CT;
                    -- last: we will send a TAG afterwards, this is never
                    -- the last segment.
                    do_data_internal_flags(0) <= '0';
                end if;

                do_data_internal_flags(3) <= '0';      -- Partial = '0',
                -- XXX: The definition for EOI is not intuitive for data out.
                --      At the moment, EOI is defined to be '0'.
                --      However, this might be change to '1' in the future!
                do_data_internal_flags(2) <= '0';      --EOI
                do_data_internal_flags(1) <=  cmd(25); --EOT

                -- reserved not used.
                do_data_internal_reserved <= (others => '0');
                -- length forwarded from the cmd FIFO
                do_data_internal_length   <= cmd(15 downto 0);

            when S_OUT_MSG =>
                bdo_ready_p       <= do_ready;
                do_valid_internal <= bdo_valid_p;
                do_data_internal  <= bdo_p;
                en_SegLenCnt      <= bdo_valid_p and do_ready;

            --TAG
            when S_HDR_TAG =>
                do_valid_internal         <= '1';
                do_data_internal_opcode   <= HDR_TAG;
                -- Partial = '0', EOI ='0', EOT = '1', Last = '1':
                -- No tag is larger than 2^(16-1) bytes
                do_data_internal_flags    <= "0011";
                do_data_internal_reserved <= (others => '0'); --reserved not used.
                do_data_internal_length   <= std_logic_vector(to_unsigned(TAGdiv8, 16));

            when S_OUT_TAG =>
                bdo_ready_p       <= do_ready;
                do_valid_internal <= bdo_valid_p;
                do_data_internal  <= bdo_p;

            when S_VER_TAG =>
                msg_auth_ready <= '1';

            --HASH-VALUE
            when S_HDR_HASH_VALUE =>
                do_valid_internal         <= '1';
                do_data_internal_opcode   <= HDR_HASH_VALUE;
                -- Partial = '0', EOI ='0', EOT = '1', LAST = '1':
                -- No tag is larger than 2^(16-1) bytes
                do_data_internal_flags    <= "0011";
                do_data_internal_reserved <= (others => '0'); -- reserved not used
                do_data_internal_length   <= std_logic_vector(to_unsigned(HASHdiv8, 16));

            when S_OUT_HASH_VALUE =>
                bdo_ready_p       <= do_ready;
                do_valid_internal <= bdo_valid_p;
                do_data_internal  <= bdo_p;

            --STATUS
            when S_STATUS_FAIL =>
                do_valid_internal             <= '1';
                -- do_last must only be asserted together with do_valid(_internal)
                do_last                       <= '1';
                do_data_internal_opcode       <= INST_FAILURE;
                do_data_internal(27 downto 0) <= (others=>'0');

            when S_STATUS_SUCCESS =>
                do_valid_internal             <= '1';
                -- do_last must only be asserted together with do_valid(_internal)
                do_last                       <= '1';
                do_data_internal_opcode       <= INST_SUCCESS;
                do_data_internal(27 downto 0) <= (others=>'0');

            when others=>
                null;

        end case;
    end process;

end generate;

end PostProcessor;
