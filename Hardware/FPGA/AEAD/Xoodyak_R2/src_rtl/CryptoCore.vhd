--------------------------------------------------------------------------------
--! @file       CryptoCore.vhd
--! @brief      Implementation of the xoodyak cipher without hashing.
--!
--! @author     Silvia Mella <silvia.mella@st.com>
--! @license    To the extent possible under law, the implementer has waived all copyright
--              and related or neighboring rights to the source code in this file.
--              http://creativecommons.org/publicdomain/zero/1.0/
--! @note       This code is based on the package for the dummy cipher provided within
--!             the Development Package for Hardware Implementations Compliant with
--!             the Hardware API for Lightweight Cryptography (https://github.com/GMUCERG/LWC)
--------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.std_logic_misc.all;
use work.NIST_LWAPI_pkg.all;
use work.design_pkg.all;
use work.xoodoo_globals.all;


entity CryptoCore is
    Port (
        clk             : in   STD_LOGIC;
        rst             : in   STD_LOGIC;
        --PreProcessor===============================================
        ----!key----------------------------------------------------
        key             : in   STD_LOGIC_VECTOR (CCSW     -1 downto 0);
        key_valid       : in   STD_LOGIC;
        key_ready       : out  STD_LOGIC;
        ----!Data----------------------------------------------------
        bdi             : in   STD_LOGIC_VECTOR (CCW     -1 downto 0);
        bdi_valid       : in   STD_LOGIC;
        bdi_ready       : out  STD_LOGIC;
        bdi_pad_loc     : in   STD_LOGIC_VECTOR (CCWdiv8 -1 downto 0);
        bdi_valid_bytes : in   STD_LOGIC_VECTOR (CCWdiv8 -1 downto 0);
        bdi_size        : in   STD_LOGIC_VECTOR (3       -1 downto 0);
        bdi_eot         : in   STD_LOGIC;
        bdi_eoi         : in   STD_LOGIC;
        bdi_type        : in   STD_LOGIC_VECTOR (4       -1 downto 0);
        decrypt_in      : in   STD_LOGIC;
        key_update      : in   STD_LOGIC;
        hash_in         : in   std_logic;
        --!Post Processor=========================================
        bdo             : out  STD_LOGIC_VECTOR (CCW      -1 downto 0);
        bdo_valid       : out  STD_LOGIC;
        bdo_ready       : in   STD_LOGIC;
        bdo_type        : out  STD_LOGIC_VECTOR (4       -1 downto 0);
        bdo_valid_bytes : out  STD_LOGIC_VECTOR (CCWdiv8 -1 downto 0);
        end_of_block    : out  STD_LOGIC;
        msg_auth_valid  : out  STD_LOGIC;
        msg_auth_ready  : in   STD_LOGIC;
        msg_auth        : out  STD_LOGIC
    );
end CryptoCore;

architecture behavioral of CryptoCore is

    -- Xoodoo permutation

    component  xoodoo is
        generic( roundPerCycle : integer  := roundsPerCycle);
        port (
            clk_i           : in std_logic;
            rst_i           : in std_logic;
            start_i         : in std_logic;
            state_valid_o   : out std_logic;
            init_reg        : in std_logic;
            word_in         : in std_logic_vector(31 downto 0);
            word_index_in   : in integer range 0 to 11;
            word_enable_in  : in std_logic;
            domain_i        : in std_logic_vector(31 downto 0);
            domain_enable_i : in std_logic;
            word_out        : out std_logic_vector(31 downto 0)
            );
    end component;

    -- Number of words the respective blocks contain.
    constant STATE_WORDS_C      : integer   := get_words(STATE_SIZE, CCW);
    constant NPUB_WORDS_C       : integer   := get_words(NPUB_SIZE, CCW);
    --constant HASH_WORDS_C       : integer   := get_words(HASH_VALUE_SIZE, CCW);
    constant BLOCK_WORDS_C      : integer   := get_words(DBLK_SIZE, CCW);
    constant RKIN_WORDS_C       : integer   := get_words(RKIN, CCW);
    constant RKOUT_WORDS_C      : integer   := get_words(RKOUT, CCW);
    constant KEY_WORDS_C        : integer   := get_words(KEY_SIZE, CCW);
    constant TAG_WORDS_C        : integer   := get_words(TAG_SIZE, CCW);

    -- State signals
    type state_t is (IDLE,
                    STORE_KEY,
                    PADD_KEY,
                    ABSORB_NONCE,
                    PADD_NONCE,
                    ABSORB_AD,
                    PADD_AD,
                    PADD_AD_ONLY_DOMAIN,
                    PADD_AD_BLOCK,
                    ABSORB_MSG,
                    PADD_MSG,
                    PADD_MSG_ONLY_DOMAIN,
                    PADD_MSG_BLOCK,
                    EXTRACT_TAG,
                    VERIFY_TAG,
                    WAIT_ACK);

    -- FSM signals
    signal n_state_s, state_s : state_t;

    -- Word counter for address generation. Increases every time a word is transferred.
    signal word_cnt_s                   : integer range 0 to STATE_WORDS_C - 1;

    -- Internal Port signals
    signal key_s                        : std_logic_vector(CCSW - 1 downto 0);
    signal key_ready_s                  : std_logic;
    signal bdi_ready_s                  : std_logic;
    signal bdi_s                        : std_logic_vector(CCW - 1 downto 0);
    signal bdi_valid_bytes_s            : std_logic_vector(CCWdiv8 - 1 downto 0);
    signal bdi_pad_loc_s                : std_logic_vector(CCWdiv8 - 1 downto 0);

    signal bdo_s                        : std_logic_vector(CCW - 1 downto 0);
    signal bdo_valid_bytes_s            : std_logic_vector(CCWdiv8 - 1 downto 0);
    signal bdo_valid_s                  : std_logic;
    signal bdo_type_s                   : std_logic_vector(3 downto 0);
    signal end_of_block_s               : std_logic;
    signal msg_auth_valid_s             : std_logic;
    signal tag_ready_s                  : std_logic;

    -- Internal flags
    signal bdi_partial_s                  : std_logic;
    signal n_decrypt_s, decrypt_s         : std_logic;
    signal n_msg_auth_s, msg_auth_s       : std_logic;
    signal n_eoi_s, eoi_s                 : std_logic;
    signal n_update_key_s, update_key_s   : std_logic;
    signal n_first_block_s, first_block_s : std_logic;

    -- Xoodoo signals
    signal xoodoo_start_s, n_xoodoo_start_s, xoodoo_valid_s: std_logic;

    signal word_in_s        : std_logic_vector(31 downto 0);
    signal word_index_in_s  : integer range 0 to 11;
    signal word_enable_in_s : std_logic;
    signal init_reg_s       : std_logic;
    signal padd_s           : std_logic_vector(31 downto 0);
    signal padd_enable_s    : std_logic;
    signal xoodoo_state_word_s : std_logic_vector(31 downto 0);

    -- Xoodyak signals
    signal domain_s, n_domain_s : std_logic_vector(CCW-1 downto 0);


begin
    ----------------------------------------------------------------------------
    -- port map of Xoodoo component
    ----------------------------------------------------------------------------
    i_xoodoo: xoodoo
        generic map (
            roundPerCycle => roundsPerCycle
        )
        port map (
            clk_i => clk,
            rst_i => rst,
            start_i => xoodoo_start_s,
            state_valid_o => xoodoo_valid_s,
            init_reg => init_reg_s,
            word_in => word_in_s,
            word_index_in => word_index_in_s,
            word_enable_in => word_enable_in_s,
            domain_i          => padd_s,
            domain_enable_i   => padd_enable_s,
            word_out => xoodoo_state_word_s
        );

    word_index_in_s <= word_cnt_s;

    ----------------------------------------------------------------------------
    -- I/O Mappings
    ----------------------------------------------------------------------------

    -- little endian
    key_s               <= reverse_byte(key);
    bdi_s               <= reverse_byte(bdi);
    bdi_valid_bytes_s   <= reverse_bit(bdi_valid_bytes);
    bdi_pad_loc_s       <= reverse_bit(bdi_pad_loc);
    key_ready           <= key_ready_s;
    bdi_ready           <= bdi_ready_s;
    bdo                 <= reverse_byte(bdo_s);
    bdo_valid_bytes     <= reverse_bit(bdo_valid_bytes_s);
    bdo_valid           <= bdo_valid_s;
    bdo_type            <= bdo_type_s;
    end_of_block        <= end_of_block_s;
    msg_auth            <= msg_auth_s;
    msg_auth_valid      <= msg_auth_valid_s;

    -- big endian

    -- key_s               <= key;
    -- bdi_s               <= bdi;
    -- bdi_valid_bytes_s   <= bdi_valid_bytes;
    -- bdi_pad_loc_s       <= bdi_pad_loc;
    -- key_ready           <= key_ready_s;
    -- bdi_ready           <= bdi_ready_s;
    -- bdo                 <= bdo_s;
    -- bdo_valid_bytes     <= bdo_valid_bytes_s;
    -- bdo_valid           <= bdo_valid_s;
    -- bdo_type            <= bdo_type_s;
    -- end_of_block        <= end_of_block_s;
    -- msg_auth            <= msg_auth_s;
    -- msg_auth_valid      <= msg_auth_valid_s;

    -- Utility signal: Indicates whether the input word is fully filled or not.
    -- If '1', word is only partially filled.
    -- Used to determine whether 0x01 padding word can be inserted into this last word.
    bdi_partial_s <= or_reduce(bdi_pad_loc_s);

    ----------------------------------------------------------------------------
    --! Bdo multiplexer
    ----------------------------------------------------------------------------
    p_bdo_mux : process(state_s, bdi_s, word_cnt_s,
                        bdi_valid_bytes_s, bdi_valid, bdi_eot, decrypt_s,
                        xoodoo_state_word_s,
                        bdi_ready_s, xoodoo_valid_s, xoodoo_start_s)
    begin
        -- Default values preventing latches
        bdo_s               <= (others => '0');
        bdo_valid_bytes_s   <= '1'&(CCWdiv8 - 2 downto 0 => '0');
        bdo_valid_s         <= '0';
        end_of_block_s      <= '0';
        bdo_type_s          <= HDR_TAG;

        case state_s is
            -- Connect bdo signals and encryp/decrypt data.
            -- Set bdo_type depending on mode.
            when ABSORB_MSG =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                bdo_s               <= bdi_s xor xoodoo_state_word_s;
                bdo_valid_bytes_s   <= bdi_valid_bytes_s;
                bdo_valid_s         <= bdi_valid and bdi_ready_s;
                end_of_block_s      <= bdi_eot;
                if (decrypt_s = '1') then
                    bdo_type_s <= HDR_PT;
                else
                    bdo_type_s <= HDR_CT;
                end if;
                end if;

            -- Set end_of_block_s on the last word of the tag block
            when EXTRACT_TAG =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                bdo_s               <= xoodoo_state_word_s;
                bdo_valid_bytes_s   <= (others => '1');
                bdo_valid_s         <= '1';
                    bdo_type_s <= HDR_TAG;
                    if (word_cnt_s = TAG_WORDS_C - 1) then
                    end_of_block_s <= '1';
                else
                    end_of_block_s <= '0';
                end if;
                end if;

            -- Default values.
            when others =>
                bdo_s               <= (others => '0');
                bdo_valid_bytes_s   <= '1'&(CCWdiv8 - 2 downto 0 => '0');
                bdo_valid_s         <= '0';
                end_of_block_s      <= '0';
                bdo_type_s          <= HDR_TAG;

        end case;
    end process p_bdo_mux;

    ----------------------------------------------------------------------------
    --! Registers for state and internal signals
    ----------------------------------------------------------------------------
    p_reg : process(clk)
    begin
        if rising_edge(clk) then
            if (rst = active_rst_p) then
                msg_auth_s            <= '1';
                eoi_s                 <= '0';
                update_key_s          <= '0';
                decrypt_s             <= '0';
                state_s               <= IDLE;
                first_block_s         <= '1';
                domain_s              <= (others=>'0');
                xoodoo_start_s        <= '0';
            else
                msg_auth_s            <= n_msg_auth_s;
                eoi_s                 <= n_eoi_s;
                update_key_s          <= n_update_key_s;
                decrypt_s             <= n_decrypt_s;
                state_s               <= n_state_s;
                first_block_s         <= n_first_block_s;
                domain_s              <= n_domain_s;
                xoodoo_start_s        <= n_xoodoo_start_s;
            end if;
        end if;
    end process p_reg;

    ----------------------------------------------------------------------------
    --! Next_state FSM
    ----------------------------------------------------------------------------
    p_next_state : process(state_s, key_valid, key_ready_s, key_update, bdi_valid, hash_in,
                             bdi_ready_s, bdi_eot, bdi_eoi, eoi_s, bdi_type, bdi_pad_loc_s,
                             word_cnt_s,
                             decrypt_s, bdo_valid_s, bdo_ready,
                             msg_auth_valid_s, msg_auth_ready, bdi_partial_s, xoodoo_valid_s,
                             xoodoo_start_s, first_block_s)

    begin
        -- Default values preventing latches
        n_state_s <= state_s;

        case state_s is
            -- Wakeup as soon as valid bdi or key is signaled.
            when IDLE =>
                if (key_valid = '1' or bdi_valid = '1') then
                    if (hash_in = '1') then
                        n_state_s <= IDLE;
                    else
                        n_state_s <= STORE_KEY;
                    end if;
                else
                    n_state_s <= IDLE;
                end if;

            -- Wait until the new key is completely received.
            -- It is assumed, that key is only updated if Npub follows. Otherwise
            -- state transition back to IDLE is required to prevent deadlock in case
            -- bdi data follows that is not of type npub.
            when STORE_KEY =>
                if (((key_valid = '1' and key_ready_s = '1') or key_update = '0')
                and word_cnt_s >= KEY_WORDS_C - 1) then
                    n_state_s <= PADD_KEY;
                else
                    n_state_s <= STORE_KEY;
                end if;

            when PADD_KEY =>
                n_state_s <= ABSORB_NONCE;

            -- Wait until the whole nonce block is received. If no npub
            -- follows, directly go to extracting/verifying tag.
            when ABSORB_NONCE =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    if (bdi_valid = '1' and bdi_ready_s = '1' and word_cnt_s >= NPUB_WORDS_C - 1) then
                        n_state_s <= PADD_NONCE;
                    else
                        n_state_s <= ABSORB_NONCE;
                    end if;
                end if;

            when PADD_NONCE =>
                n_state_s <= ABSORB_AD;

            -- In case input is plaintext or ciphertext, no ad is processed.
            -- Wait until last word of AD is signaled. If padding is required
            -- (word_cnt_s < BLOCK_WORDS_C - 1) and 0x01 padding is not inserted
            -- in last word yet (bdi_partial_s = '0') go to padding state, else
            -- either go to absorb msg or directly aborb length depending on presence of msg data.
            when ABSORB_AD =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    if (eoi_s = '1') then -- empty ad and empty msg
                        n_state_s <= PADD_AD;
                    else
                        if (bdi_valid = '1' and (bdi_type = HDR_PT or bdi_type = HDR_CT)) then
                            n_state_s <= PADD_AD;
                        elsif (bdi_valid = '1' and bdi_ready_s = '1' and bdi_eot = '1') then
                            if (word_cnt_s < RKIN_WORDS_C) then
                                if (bdi_partial_s = '0') then -- 10 has not been added in last word
                                    n_state_s <= PADD_AD; -- padd is done in the new word
                                else
                                    n_state_s <= PADD_AD_ONLY_DOMAIN;
                                end if;
                            else
                                n_state_s <= PADD_AD;
                            end if;
                        elsif (bdi_valid = '1' and bdi_ready_s = '1' and word_cnt_s >= RKIN_WORDS_C - 1) then
                            n_state_s <= PADD_AD_BLOCK; -- this then calls xoodoo
                        else
                            n_state_s <= ABSORB_AD;
                        end if;
                    end if;
                else
                    n_state_s <= ABSORB_AD;
                end if;

            -- Since only one cycle is required to insert 0x01 padding byte,
            -- go to absorb msg state or directly absorb length depending on the
            -- presence of msg data.
            when PADD_AD =>
                if (eoi_s = '1') then -- empty msg
                    n_state_s <= PADD_MSG;
                else
                    n_state_s <= ABSORB_MSG;
                end if;

            when PADD_AD_ONLY_DOMAIN =>
                if (eoi_s = '1') then -- empty msg
                    n_state_s <= PADD_MSG;
                else
                    n_state_s <= ABSORB_MSG;
                end if;

            when PADD_AD_BLOCK =>
                n_state_s <= ABSORB_AD;

            -- Read in either plaintext or ciphertext until end of type is
            -- detected. Then check whether padding is necessary or not as previously.
            when ABSORB_MSG =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    if (bdi_valid = '1' and bdi_ready_s = '1' and bdi_eot = '1') then
                        if (word_cnt_s < RKOUT_WORDS_C) then
                            if (bdi_partial_s = '0') then -- 10 has not been added in last word
                                n_state_s <= PADD_MSG; -- padd is done in the new word
                            else
                                n_state_s <= PADD_MSG_ONLY_DOMAIN;
                            end if;
                        else
                            n_state_s <= PADD_MSG;
                        end if;
                    elsif (bdi_valid = '1' and bdi_ready_s = '1' and word_cnt_s >= RKOUT_WORDS_C - 1) then
                        n_state_s <= PADD_MSG_BLOCK; -- this then calls xoodoo
                    else
                        n_state_s <= ABSORB_MSG;
                    end if;
                else
                    n_state_s <= ABSORB_MSG;
                end if;

            when PADD_MSG =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    if (decrypt_s = '1') then
                        n_state_s <= VERIFY_TAG;
                    else
                        n_state_s <= EXTRACT_TAG;
                    end if;
                else
                    n_state_s <= PADD_MSG;
                end if;

            when PADD_MSG_ONLY_DOMAIN =>
                if (decrypt_s = '1') then
                    n_state_s <= VERIFY_TAG;
                else
                    n_state_s <= EXTRACT_TAG;
                end if;

            when PADD_MSG_BLOCK =>
                n_state_s <= ABSORB_MSG;


            -- Wait until the whole tag block is transferred, then go back to IDLE.
            when EXTRACT_TAG =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    if (bdo_valid_s = '1' and bdo_ready = '1' and word_cnt_s >= TAG_WORDS_C - 1) then
                        n_state_s <= IDLE;
                    else
                        n_state_s <= EXTRACT_TAG;
                    end if;
                else
                    n_state_s <= EXTRACT_TAG;
                end if;

            -- Wait until the tag being verified is received, continue
            -- with waiting for acknowledgement on msg_auth_valid.
            when VERIFY_TAG =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    if (bdi_valid = '1' and bdi_ready_s = '1' and word_cnt_s >= TAG_WORDS_C - 1) then
                        n_state_s <= WAIT_ACK;
                    else
                        n_state_s <= VERIFY_TAG;
                    end if;
                else
                    n_state_s <= VERIFY_TAG;
                end if;

            -- Wait until message authentication is acknowledged.
            when WAIT_ACK =>
                if (msg_auth_valid_s = '1' and msg_auth_ready = '1') then
                    n_state_s <= IDLE;
                else
                    n_state_s <= WAIT_ACK;
                end if;

            when others =>
                n_state_s <= IDLE;

        end case;
    end process p_next_state;


    ----------------------------------------------------------------------------
    --! Decoder process for control logic
    ----------------------------------------------------------------------------
    p_decoder : process(state_s, n_state_s, key_valid, key_ready_s, key_update, update_key_s, key_s,
                            bdi_s, bdi_valid, bdi_ready_s, bdi_eoi, bdi_valid_bytes_s, bdi_pad_loc_s,
                            bdi_size, bdi_type, eoi_s, decrypt_in, decrypt_s,
                            bdo_s, bdo_ready, word_cnt_s, msg_auth_s, msg_auth_valid_s, msg_auth_ready,
                            first_block_s,
                            domain_s, xoodoo_valid_s, xoodoo_start_s,
                            xoodoo_state_word_s)
    begin
        -- Default values preventing latches
        key_ready_s            <= '0';
        bdi_ready_s            <= '0';
        msg_auth_valid_s       <= '0';
        n_msg_auth_s           <= msg_auth_s;
        n_eoi_s                <= eoi_s;
        n_update_key_s         <= update_key_s;
        n_decrypt_s            <= decrypt_s;
        n_first_block_s        <= first_block_s;
        n_domain_s             <= domain_s;
        tag_ready_s            <= '0';
        init_reg_s             <= '0';
        word_in_s              <= (others => '0');
        word_enable_in_s       <= '0';
        padd_s              <= (others => '0');
        padd_enable_s       <= '0';
        n_xoodoo_start_s       <= '0';

        case state_s is

            -- Default values. If valid input is detected, set internal flags
            -- depending on input and mode.
            when IDLE =>
                n_msg_auth_s    <= '1';
                n_eoi_s         <= '0';
                n_decrypt_s     <= '0';
                n_first_block_s <= '1';
                init_reg_s      <= '1';
                if (key_valid = '1' and key_update = '1') then
                    n_update_key_s  <= '1';
                end if;

            -- If key must be updated, assert key_ready.
            when STORE_KEY =>
                if (update_key_s = '1') then
                    key_ready_s <= '1';
                end if;
                if ((key_valid = '1' and key_ready_s = '1')) then
                    word_in_s <= key_s;
                    word_enable_in_s <= '1';
                end if;
                n_domain_s <= DOMAIN_ABSORB_KEY;

            when PADD_KEY =>
                word_in_s <= PADD_01_KEY;
                word_enable_in_s <= '1';
                padd_s <= domain_s;
                padd_enable_s <= '1';
                n_xoodoo_start_s <= '1';
                n_domain_s <= DOMAIN_ABSORB;

            -- Store bdi_eoi (will only be effective on last word) and decrypt_in flag.
            when ABSORB_NONCE =>
                if (xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    bdi_ready_s     <= '1';
                end if;
                n_eoi_s         <= bdi_eoi;
                n_decrypt_s     <= decrypt_in;
                if (bdi_valid = '1' and bdi_ready_s = '1' and bdi_type = HDR_NPUB) then
                    word_in_s <= bdi_s;
                    word_enable_in_s <= '1';
                    n_eoi_s     <= bdi_eoi;
                end if;

            when PADD_NONCE =>
                word_in_s <= PADD_01;
                word_enable_in_s <= '1';
                padd_s <= domain_s;
                padd_enable_s <= '1';
                n_xoodoo_start_s <= '1';

            -- If pt or ct is detected, don't assert bdi_ready, otherwise first word
            -- gets lost. Store bdi_eoi.
            -- padd() returns vector containing bdi_s and inserted 0x01 byte according
            -- to 0100* padding.
            when ABSORB_AD =>
                if(eoi_s = '0') then
                    if not (bdi_valid = '1' and (bdi_type = HDR_PT or bdi_type = HDR_CT)) then
                        if (xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                            bdi_ready_s <= '1';
                        end if;
                    end if;
                    if (bdi_valid = '1' and bdi_ready_s = '1') then
                        n_eoi_s         <= bdi_eoi;
                        if (bdi_type = HDR_AD) then
                            word_in_s <= padd(bdi_s, bdi_valid_bytes_s, bdi_pad_loc_s);
                            word_enable_in_s <= '1';
                        end if;
                    end if;
                end if;

            -- Insert 0x01 padding byte (state is only reached if not yet inserted).
            when PADD_AD  =>
                if (word_cnt_s < STATE_WORDS_C-1 ) then
                    word_in_s <= PADD_01;
                    word_enable_in_s <= '1';
                    padd_s <= domain_s xor DOMAIN_CRYPT;
                    padd_enable_s <= '1';
                else
                    padd_s <= PADD_01 xor domain_s xor DOMAIN_CRYPT;
                    padd_enable_s <= '1';
                end if;
                n_domain_s <= DOMAIN_ZERO;
                n_xoodoo_start_s <= '1';
                --n_first_block_s <= '0';

            when PADD_AD_ONLY_DOMAIN =>
                padd_s <= domain_s xor DOMAIN_CRYPT;
                padd_enable_s <= '1';
                n_domain_s <= DOMAIN_ZERO;
                --n_first_block_s <= '0';
                n_xoodoo_start_s <= '1';

            when PADD_AD_BLOCK =>
                padd_s <= PADD_01 xor domain_s;
                padd_enable_s <= '1';
                n_domain_s <= DOMAIN_ZERO;
                n_xoodoo_start_s <= '1';

            -- Generate CT / PT
            when ABSORB_MSG =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    bdi_ready_s <= bdo_ready;
                end if;
                if (bdi_valid = '1' and bdi_ready_s = '1') then
                    if (decrypt_s = '0' and bdi_type = HDR_PT) then
                        word_in_s <= padd(bdi_s, bdi_valid_bytes_s, bdi_pad_loc_s);
                        word_enable_in_s <= '1';
                    elsif(bdi_type = HDR_CT) then
                        word_in_s <= (xoodoo_state_word_s and not(select_bytes(bdi_s, bdi_valid_bytes_s))) xor padd(bdi_s, bdi_valid_bytes_s, bdi_pad_loc_s);
                        word_enable_in_s <= '1';
                    end if;
                end if;

            when PADD_MSG =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    word_in_s <= PADD_01;
                    word_enable_in_s <= '1';
                    n_xoodoo_start_s <= '1';
                    padd_s <= DOMAIN_SQUEEZE;
                    padd_enable_s <= '1';
                end if;

            when PADD_MSG_ONLY_DOMAIN =>
                n_xoodoo_start_s <= '1';
                padd_s <= DOMAIN_SQUEEZE;
                padd_enable_s <= '1';

            when PADD_MSG_BLOCK =>
                word_in_s <= PADD_01;
                word_enable_in_s <= '1';
                n_xoodoo_start_s <= '1';

            when EXTRACT_TAG =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    tag_ready_s <= '1';
                end if;

            -- As soon as bdi input doesn't match with calculated tag in ram,
            -- reset msg_auth.
            when VERIFY_TAG =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    bdi_ready_s <= '1';
                end if;
                if (bdi_valid = '1' and bdi_ready_s = '1' and bdi_type = HDR_TAG) then
                    if (bdi_s /= xoodoo_state_word_s) then
                        n_msg_auth_s <= '0';
                    end if;
                end if;

            -- Signal msg auth valid.
            when WAIT_ACK =>
                msg_auth_valid_s <= '1';

            when others =>
                null;

        end case;
    end process p_decoder;


    ----------------------------------------------------------------------------
    --! Word, Byte and Block counters
    ----------------------------------------------------------------------------
    p_counters : process(clk)
    begin
        if rising_edge(clk) then
                case state_s is
                    -- Nothing to do here, reset counters
                    when IDLE =>
                       word_cnt_s      <= 0;

                    -- If key is to be updated, increase counter on every successful
                    -- data transfer (valid and ready).
                    when STORE_KEY =>
                        if (key_update = '1') then
                            if (key_valid = '1' and key_ready_s = '1') then
                                if (word_cnt_s > KEY_WORDS_C - 1) then
                                    word_cnt_s <= 0;
                                else
                                    word_cnt_s <= word_cnt_s + 1;
                                end if;
                            end if;
                        else
                            if (word_cnt_s > KEY_WORDS_C - 1) then
                                word_cnt_s <= 0;
                            else
                                word_cnt_s <= word_cnt_s + 1;
                            end if;
                        end if;

                    when PADD_KEY =>
                        word_cnt_s <= 0;

                    -- Every time a word is transferred, increase counter
                    -- up to NPUB_WORDS_C
                    when ABSORB_NONCE =>
                        if (bdi_valid = '1' and bdi_ready_s = '1') then
                            if (word_cnt_s > NPUB_WORDS_C - 1) then
                                word_cnt_s <= 0;
                            else
                                word_cnt_s <= word_cnt_s + 1;
                            end if;
                        end if;

                    when PADD_NONCE =>
                        word_cnt_s <= 0;

                    -- On valid transfer, increase word counter until either
                    -- the block size is reached or the last input word is obtained and the 0x01
                    -- padding byte can already be inserted (indicated by last transfer being
                    -- only partially filled -> bdi_eot and bdi_partial).
                    when ABSORB_AD =>
                        if (bdi_valid = '1' and bdi_ready_s = '1') then
                            if (word_cnt_s > RKIN_WORDS_C - 1 or (bdi_eot = '1' and bdi_partial_s = '1')) then
                                word_cnt_s <= 0;
                            else
                                word_cnt_s <= word_cnt_s + 1;
                            end if;
                        end if;

                    -- Increase word counter when transferring data.
                    -- Reset word counter if block size is reached or the last msg word is received
                    -- and only partially filled such that the 0x01 padding byte could be inserted.
                    when ABSORB_MSG =>
                        if (bdi_valid = '1' and bdi_ready_s = '1') then
                            if (word_cnt_s > RKOUT_WORDS_C - 1 or (bdi_eot = '1' and bdi_partial_s = '1')) then
                                word_cnt_s  <= 0;
                            else
                                word_cnt_s <= word_cnt_s + 1;
                            end if;
                        end if;

                    -- Reset word counters here.
                when PADD_AD | PADD_MSG  | PADD_AD_BLOCK | PADD_AD_ONLY_DOMAIN | PADD_MSG_BLOCK | PADD_MSG_ONLY_DOMAIN  =>
                        word_cnt_s <= 0;

                    -- Increase word counter on valid bdo transfer until block size is reached.
                when EXTRACT_TAG =>
                        if (tag_ready_s = '1' and bdo_ready = '1') then
                        if (word_cnt_s >= TAG_WORDS_C - 1) then
                                word_cnt_s  <= 0;
                            else
                                word_cnt_s <= word_cnt_s + 1;
                            end if;
                        end if;

                    -- Increase word counter when transferring the received data (tag).
                    when VERIFY_TAG =>
                        if (bdi_valid = '1' and bdi_ready_s = '1') then
                            if (word_cnt_s >= TAG_WORDS_C - 1) then
                                word_cnt_s  <= 0;
                            else
                                word_cnt_s <= word_cnt_s + 1;
                            end if;
                        end if;

                    when others =>
                        null;

                end case;
        end if;
    end process p_counters;

end behavioral;
