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
    component  xoodoo_1rnd is
        generic( roundPerCycle : integer  := 1);
        port (
            clk_i           : in std_logic;
            rst_i           : in std_logic;
            start_i         : in std_logic;
            state_i         : in std_logic_vector(383 downto 0); -- in   x_state_type;
            state_o         : out std_logic_vector(383 downto 0); -- out x_state_type;
            state_valid_o   : out std_logic
            );
    end component;

    --! Constant to check for empty hash
    constant EMPTY_HASH_SIZE_C  : std_logic_vector(2 downto 0)  := (others => '0');
    -- Counters are specified to be 64-Bit counters for AD/MSG-Bits. We only count
    -- Bytes and prepend three zero bits afterwards to save FFs (thus max range is 61).
    --! Width of the ad byte-counter.
    --constant AD_CNT_WIDTH_C     : integer range 1 to 61 := AD_CNT_WIDTH - 3;
    --! Width of msg byte-counter
    --constant MSG_CNT_WIDTH_C    : integer range 1 to 61 := MSG_CNT_WIDTH - 3;
    --! Width for the msg block counter.
    -- Has to count up to (#MSG_BYTES + DBLK_SIZE/8 - 1) / (DBLK_SIZE/8) which is equal
    -- to MSG_CNT_WIDTH_C - log2_ceil(DBLK_SIZE/8) bits. But at least use one bit.
    --constant BLOCK_CNT_WIDTH_C  : integer   := max(MSG_CNT_WIDTH_C - log2_ceil(DBLK_SIZE/8), 1);

    -- Number of words the respective blocks contain.
    constant STATE_WORDS_C      : integer   := get_words(STATE_SIZE, CCW);
    constant NPUB_WORDS_C       : integer   := get_words(NPUB_SIZE, CCW);
    constant HASH_WORDS_C       : integer   := get_words(HASH_VALUE_SIZE, CCW);
    constant BLOCK_WORDS_C      : integer   := get_words(DBLK_SIZE, CCW);
    constant RKIN_WORDS_C       : integer   := get_words(RKIN, CCW);
    constant RKOUT_WORDS_C      : integer   := get_words(RKOUT, CCW);
    constant KEY_WORDS_C        : integer   := get_words(KEY_SIZE, CCW);
    --constant LANE_WORDS_C       : integer   := get_words(LANE_SIZE, CCW);
    constant TAG_WORDS_C        : integer   := get_words(TAG_SIZE, CCW);

    -- State signals
    type state_t is (IDLE,
                    STORE_KEY,
                    PADD_KEY,
                    PRE_ABSORB_NONCE,
                    ABSORB_NONCE,
                    PADD_NONCE,
                    RUN_XOODOO,
                    --SAMPLE_XOODOO_OUT,
                    PRE_ABSORB_AD,
                    ABSORB_AD,
                    PADD_AD,
                    PADD_AD_ONLY_DOMAIN,
                    PADD_AD_BLOCK,
                    PRE_ABSORB_MSG,
                    ABSORB_MSG,
                    PADD_MSG,
                    PADD_MSG_ONLY_DOMAIN,
                    PADD_MSG_BLOCK,
                    --ABSORB_LENGTH,
                    PRE_EXTRACT_TAG,
                    EXTRACT_TAG,
                    VERIFY_TAG,
                    WAIT_ACK,
                    INIT_HASH,
                    ABSORB_HASH_MSG,
                    PADD_HASH_MSG,
                    EXTRACT_HASH_VALUE);
    -- FSM signals                
    signal n_state_s, state_s : state_t;
    -- Signals that store the next state after xoodoo permutation
    signal n_next_state_s, next_state_s : state_t;

    -- Concatenated length according to specification
    --signal len_s                        : std_logic_vector(STATE_SIZE - 1 downto 0);--std_logic_vector(DBLK_SIZE - 1 downto 0);
    --signal len_word_s                   : std_logic_vector(CCW - 1 downto 0);
    -- Number of ad / msg bytes and their corresponding bit vectors (required for casting...)
    --signal ad_byte_cnt_s                : unsigned(AD_CNT_WIDTH_C - 1 downto 0);
    --signal ad_bit_cnt_vec_s             : std_logic_vector(STATE_SIZE/2 - 1 downto 0);--std_logic_vector(DBLK_SIZE/2 - 1 downto 0);
    --signal msg_byte_cnt_s               : unsigned(MSG_CNT_WIDTH_C - 1 downto 0);
    --signal msg_bit_cnt_vec_s            : std_logic_vector(STATE_SIZE/2 - 1 downto 0);--std_logic_vector(DBLK_SIZE/2 - 1 downto 0);
    -- Counter for blocks will be casted to block number block_num_s according to specification
    --signal block_cnt_s                  : unsigned(BLOCK_CNT_WIDTH_C - 1 downto 0);
    --signal block_num_s                  : std_logic_vector(STATE_SIZE - 1 downto 0);--std_logic_vector(DBLK_SIZE - 1 downto 0);
    --signal block_num_word_s             : std_logic_vector(CCW - 1 downto 0);

    -- Word counter for address generation. Increases every time a word is transferred.
    signal word_cnt_s                   : integer range 0 to STATE_WORDS_C - 1;
    --signal ram_addr_s                   : std_logic_vector(ADDR_BITS_256_C - 1 downto 0);

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
    signal n_hash_s, hash_s               : std_logic;
    signal n_empty_hash_s, empty_hash_s   : std_logic;
    --signal n_empty_ad_s, empty_ad_s       : std_logic;
    signal n_msg_auth_s, msg_auth_s       : std_logic;
    signal n_eoi_s, eoi_s                 : std_logic;
    signal n_update_key_s, update_key_s   : std_logic;
    signal n_first_block_s, first_block_s : std_logic;

    -- Xoodoo signals
    signal xoodoo_start_s, n_xoodoo_start_s, xoodoo_valid_s: std_logic;
    signal xoodoo_state_in_s, xoodoo_state_out_s : std_logic_vector(383 downto 0);--x_state_type;

    type x_state_array is array (0 to STATE_WORDS_C-1) of std_logic_vector(CCW-1 downto 0);
    signal xoodoo_state_array_s, n_xoodoo_state_array_s : x_state_array;
    signal xoodoo_array_in_s, xoodoo_array_out_s : x_state_array;

    --signal xoodoo_state_word_s : std_logic_vector(CCW-1 downto 0);

    -- Xoodyak signals
    --signal phase : std_logic;
    signal domain_s, n_domain_s : std_logic_vector(CCW-1 downto 0);


begin
    ----------------------------------------------------------------------------
    -- port map of Xoodoo component
    ----------------------------------------------------------------------------
    i_xoodoo: xoodoo_1rnd
        generic map (
            roundPerCycle => 1
        )
        port map (
            clk_i => clk,
            rst_i => rst,
            start_i => xoodoo_start_s,
            state_i => xoodoo_state_in_s,
            state_o => xoodoo_state_out_s,
            state_valid_o => xoodoo_valid_s
        );

    -- word extracted from state to compute output
    --xoodoo_state_word_s <= xoodoo_state_array_s(word_cnt_s) when (state_s = ABSORB_MSG or state_s = EXTRACT_TAG or state_s = VERIFY_TAG) else
    --                       (others => '0');

    i00: for i in 0 to STATE_WORDS_C-1 generate
        xoodoo_state_in_s(CCW*(i+1)-1 downto CCW*i) <= xoodoo_array_in_s(i);
    end generate i00;

    o00: for i in 0 to STATE_WORDS_C-1 generate
        xoodoo_array_out_s(i) <= xoodoo_state_out_s(CCW*(i+1)-1 downto CCW*i);
    end generate o00;

    xoodoo_array_in_s <= xoodoo_state_array_s;

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
                        --block_num_word_s, xoodoo_state_word_s,
                        hash_s,  xoodoo_state_array_s, bdi_ready_s)
                        --tag_ready_s)
    begin
        case state_s is
            -- Connect bdo signals and encryp/decrypt data.
            -- Set bdo_type depending on mode.
            when ABSORB_MSG =>

                bdo_s               <= bdi_s xor xoodoo_state_array_s(word_cnt_s); --xoodoo_state_word_s;
                bdo_valid_bytes_s   <= bdi_valid_bytes_s;
                bdo_valid_s         <= bdi_valid and bdi_ready_s;--bdi_valid;
                end_of_block_s      <= bdi_eot;
                if (decrypt_s = '1') then
                    bdo_type_s <= HDR_PT;
                else
                    bdo_type_s <= HDR_CT;
                end if;

            -- Set end_of_block_s on either the last word of the tag block
            -- or the hash_value block.
            when EXTRACT_TAG | EXTRACT_HASH_VALUE =>
                bdo_s               <= xoodoo_state_array_s(word_cnt_s);
                bdo_valid_bytes_s   <= (others => '1');
                bdo_valid_s         <= '1'; --tag_ready_s;
                if (hash_s = '1') then
                    bdo_type_s <= HDR_HASH_VALUE;
                else
                    bdo_type_s <= HDR_TAG;
                end if;
                if (word_cnt_s = TAG_WORDS_C - 1 and hash_s = '0')
                or (word_cnt_s >= HASH_WORDS_C - 1 and hash_s = '1') then
                    end_of_block_s <= '1';
                else
                    end_of_block_s <= '0';
                end if;

            -- Default values.
            when others =>
                bdo_s               <= (others => '0');
                bdo_valid_bytes_s   <= '1'&(CCWdiv8 - 2 downto 0 => '0'); --(0 => '1', others => '0');
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
            if (rst = '1') then
                msg_auth_s            <= '1';
                eoi_s                 <= '0';
                update_key_s          <= '0';
                decrypt_s             <= '0';
                hash_s                <= '0';
                empty_hash_s          <= '0';
                state_s               <= IDLE;
                next_state_s          <= IDLE;
                first_block_s         <= '1';
                xoodoo_state_array_s  <= (others=>(others=>'0'));
                domain_s              <= (others=>'0');
                xoodoo_start_s        <= '0';
            else
                msg_auth_s            <= n_msg_auth_s;
                eoi_s                 <= n_eoi_s;
                update_key_s          <= n_update_key_s;
                decrypt_s             <= n_decrypt_s;
                hash_s                <= n_hash_s;
                empty_hash_s          <= n_empty_hash_s;
                state_s               <= n_state_s;
                next_state_s          <= n_next_state_s;
                first_block_s         <= n_first_block_s;
                xoodoo_state_array_s  <= n_xoodoo_state_array_s;
                domain_s              <= n_domain_s;
                xoodoo_start_s        <= n_xoodoo_start_s;
            end if;
        end if;
    end process p_reg;

    ----------------------------------------------------------------------------
    --! Next_state FSM
    ----------------------------------------------------------------------------
    p_next_state : process(state_s, next_state_s, key_valid, key_ready_s, key_update, bdi_valid,
                             bdi_ready_s, bdi_eot, bdi_eoi, eoi_s, bdi_type, bdi_pad_loc_s,
                             word_cnt_s, hash_in, decrypt_s, bdo_valid_s, bdo_ready,
                             msg_auth_valid_s, msg_auth_ready, bdi_partial_s, xoodoo_valid_s, --n_empty_ad_s,
                             xoodoo_start_s)

    begin
        -- Default values preventing latches
        n_next_state_s <= next_state_s;

        case state_s is
            -- Wakeup as soon as valid bdi or key is signaled.
            when IDLE =>
                if (key_valid = '1' or bdi_valid = '1') then
                    if (hash_in = '1') then
                        n_state_s <= IDLE; --INIT_HASH
                    else
                        n_state_s <= STORE_KEY;
                    end if;
                else
                    n_state_s <= IDLE;
                end if;
                n_next_state_s <= IDLE;
                

            -- Initialize hash with zero so we don't need padding after receiving
            -- non-full hash_msg block. Additionally no distinction between empty
            -- hash or regular hash is required when extracting hash from ram.
            when INIT_HASH =>
                if (word_cnt_s >= HASH_WORDS_C - 1) then
                    if (eoi_s = '1') then
                        n_state_s <= EXTRACT_HASH_VALUE;
                    else
                        n_state_s <= ABSORB_HASH_MSG;
                    end if;
                else
                    n_state_s <= INIT_HASH;
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
                n_state_s <= PRE_ABSORB_NONCE;
                

            when PRE_ABSORB_NONCE =>
                n_state_s <= RUN_XOODOO;
                n_next_state_s <= ABSORB_NONCE;
                

            -- Wait until the whole nonce block is received. If no npub
            -- follows, directly go to extracting/verifying tag.
            when ABSORB_NONCE =>
                if (bdi_valid = '1' and bdi_ready_s = '1' and word_cnt_s >= NPUB_WORDS_C - 1) then
                    if (bdi_eoi = '1') then
                        if (decrypt_s = '1') then
                            n_next_state_s <= PRE_ABSORB_AD;
                        else
                            n_next_state_s <= PRE_ABSORB_AD;
                        end if;
                    else
                        n_next_state_s <= PRE_ABSORB_AD;
                    end if;
                    n_state_s <= PADD_NONCE;
                else
                    n_state_s <= ABSORB_NONCE;
                end if;
                

            when PADD_NONCE =>
                n_state_s <= next_state_s;
                

            when PRE_ABSORB_AD =>
                if (eoi_s = '1') then -- empty ad and empty msg
                    n_next_state_s <= PADD_AD;
                -- elsif (bdi_valid = '1' and (bdi_type = HDR_PT or bdi_type = HDR_CT)) then -- case empty ad but msg
                    -- n_next_state_s <= PADD_AD;
                else
                    n_next_state_s <= ABSORB_AD; -- non-empty ad
                end if;
                n_state_s <= RUN_XOODOO;
                

            when RUN_XOODOO =>
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    n_state_s <= next_state_s;--SAMPLE_XOODOO_OUT;
                else
                    n_state_s <= RUN_XOODOO;
                end if;
                

            --when SAMPLE_XOODOO_OUT =>
            --    n_state_s <= next_state_s;

            -- In case input is plaintext or ciphertext, no ad is processed.
            -- Wait until last word of AD is signaled. If padding is required
            -- (word_cnt_s < BLOCK_WORDS_C - 1) and 0x01 padding is not inserted
            -- in last word yet (bdi_partial_s = '0') go to padding state, else
            -- either go to absorb msg or directly aborb length depending on presence of msg data.
            when ABSORB_AD =>
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
                

            -- Since only one cycle is required to insert 0x01 padding byte,
            -- go to absorb msg state or directly absorb length depending on the
            -- presence of msg data.
            when PADD_AD =>
                if (eoi_s = '1') then
                    n_state_s <= PRE_ABSORB_MSG; -- empty msg
                else
                    n_state_s <= PRE_ABSORB_MSG; -- non-empty msg
                end if;
                

            when PADD_AD_ONLY_DOMAIN =>
                if (eoi_s = '1') then
                    n_state_s <= PRE_ABSORB_MSG; -- empty msg
                else
                    n_state_s <= PRE_ABSORB_MSG; -- non-empty msg
                end if;
                

            when PADD_AD_BLOCK =>
                n_state_s <= PRE_ABSORB_AD;
                

            when PRE_ABSORB_MSG =>
                if (eoi_s = '1') then -- empty msg
                    n_next_state_s <= PADD_MSG;
                else
                    n_next_state_s <= ABSORB_MSG;
                end if;
                n_state_s <= RUN_XOODOO;


            -- Read in either plaintext or ciphertext until end of type is
            -- detected. Then check whether padding is necessary or not as previously.
            when ABSORB_MSG =>
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
                

            when PADD_MSG =>
                n_state_s <= PRE_EXTRACT_TAG;
                

            when PADD_MSG_ONLY_DOMAIN =>
                n_state_s <= PRE_EXTRACT_TAG;
                

            when PADD_MSG_BLOCK =>
                n_state_s <= PRE_ABSORB_MSG;
                n_next_state_s <= ABSORB_MSG;


            -- Wait for end of hash_value. Decide whether extra padding state is
            -- required or directly go to extracting hash_value state.
            when ABSORB_HASH_MSG =>
                if (bdi_valid = '1' and bdi_ready_s = '1' and bdi_eoi = '1') then
                   if (word_cnt_s < HASH_WORDS_C - 1 and bdi_partial_s = '0') then
                        n_state_s <= PADD_HASH_MSG;
                    else
                        n_state_s <= EXTRACT_HASH_VALUE;
                    end if;
                else
                    n_state_s <= ABSORB_HASH_MSG;
                end if;
                

            -- Only one cycle of padding is needed for inserting the 0x01 byte.
            -- when PADD_HASH_MSG =>
                 -- n_state_s <= EXTRACT_HASH_VALUE;
                 

            -- When lenght is absorbed, either verify the tag or extract it
            -- from ram depending on decrypt_s.
            -- when ABSORB_LENGTH =>
                -- if (word_cnt_s >= BLOCK_WORDS_C - 1) then
                    -- if (decrypt_s = '1') then
                        -- n_state_s <= VERIFY_TAG;
                    -- else
                        -- n_state_s <= EXTRACT_TAG;
                    -- end if;
                -- else
                    -- n_state_s <= ABSORB_LENGTH;
                -- end if;
                

            when PRE_EXTRACT_TAG =>
                n_state_s <= RUN_XOODOO;
                if (decrypt_s = '1') then
                    n_next_state_s <= VERIFY_TAG;
                else
                    n_next_state_s <= EXTRACT_TAG;
                end if;
                

            -- Wait until the whole tag block is transferred, then go back to IDLE.
            when EXTRACT_TAG =>
                if (bdo_valid_s = '1' and bdo_ready = '1' and word_cnt_s >= TAG_WORDS_C - 1) then
                    n_state_s <= IDLE;
                else
                    n_state_s <= EXTRACT_TAG;
                end if;
                

            -- Wait until the tag being verified is received, continue
            -- with waiting for acknowledgement on msg_auth_valid.
            when VERIFY_TAG =>
                if (bdi_valid = '1' and bdi_ready_s = '1' and word_cnt_s >= TAG_WORDS_C - 1) then
                    n_state_s <= WAIT_ACK;
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
                

            -- Wait until the whole hash_value is transferred, then go back to IDLE.
            when EXTRACT_HASH_VALUE =>
                if (bdo_valid_s = '1' and bdo_ready = '1' and word_cnt_s >= HASH_WORDS_C - 1) then
                    n_state_s <= IDLE;
                else
                    n_state_s <= EXTRACT_HASH_VALUE;
                end if;
                

            when others =>
                n_state_s <= IDLE;
                n_next_state_s <= IDLE;


        end case;
    end process p_next_state;


    ----------------------------------------------------------------------------
    --! Decoder process for control logic
    ----------------------------------------------------------------------------
    p_decoder : process(state_s, n_state_s, next_state_s, key_valid, key_ready_s, key_update, update_key_s, key_s,

                            bdi_s, bdi_valid, bdi_ready_s, bdi_eoi, bdi_valid_bytes_s, bdi_pad_loc_s,
                            bdi_size, bdi_type, eoi_s, hash_in, hash_s, empty_hash_s, decrypt_in, decrypt_s,
                            bdo_s, bdo_ready, word_cnt_s, msg_auth_s, msg_auth_valid_s, msg_auth_ready,
                            --len_word_s,
                            first_block_s, xoodoo_state_array_s, domain_s, xoodoo_valid_s, xoodoo_start_s, xoodoo_array_out_s)
    begin
        -- Default values preventing latches
        key_ready_s            <= '0';
        bdi_ready_s            <= '0';
        msg_auth_valid_s       <= '0';
        n_msg_auth_s           <= msg_auth_s;
        n_eoi_s                <= eoi_s;
        n_update_key_s         <= update_key_s;
        n_hash_s               <= hash_s;
        n_empty_hash_s         <= empty_hash_s;
        n_decrypt_s            <= decrypt_s;
        n_xoodoo_start_s       <= xoodoo_start_s;
        n_first_block_s        <= first_block_s;
        n_xoodoo_state_array_s <= xoodoo_state_array_s;
        n_domain_s             <= domain_s;
        tag_ready_s            <= '0';

        case state_s is
        
        
            -- Default values. If valid input is detected, set internal flags
            -- depending on input and mode.
            when IDLE =>
                n_msg_auth_s    <= '1';
                n_eoi_s         <= '0';
                --n_update_key_s  <= '0';
                n_hash_s        <= '0';
                n_empty_hash_s  <= '0';
                n_decrypt_s     <= '0';
                n_first_block_s <= '1';
                n_xoodoo_state_array_s <= (others => (others => '0'));
                --tag_ready_s   <= '0';
                if (key_valid = '1' and key_update = '1') then
                    n_update_key_s  <= '1';
                end if;
                if (bdi_valid = '1' and hash_in = '1') then
                    n_hash_s        <= '1';
                    if (bdi_size = EMPTY_HASH_SIZE_C) then
                        n_empty_hash_s  <= '1';
                        n_eoi_s         <= '1';
                    end if;
                end if;
                

            -- If empty hash is detected, acknowledge with one cycle bdi_ready.
            -- Afterwards empty_hash_s flag can be deasserted, it's not needed anymore.
            -- Enable tag_ram.
            when INIT_HASH =>
                if (empty_hash_s = '1') then
                    bdi_ready_s     <= '1';
                    n_empty_hash_s  <= '0';
                end if;
                

            -- If key must be updated, assert key_ready.
            when STORE_KEY =>
                if (update_key_s = '1') then
                    key_ready_s <= '1';
                    n_xoodoo_state_array_s(word_cnt_s) <= xoodoo_state_array_s(word_cnt_s) xor key_s;
                end if;
                -- if(key_valid = '1' and key_ready_s = '1') then
                    -- n_xoodoo_state_array_s(word_cnt_s) <= xoodoo_state_array_s(word_cnt_s) xor key_s;
                -- end if;
                n_domain_s <= DOMAIN_ABSORB_KEY;


            when PADD_KEY =>
                n_xoodoo_state_array_s(word_cnt_s) <= xoodoo_state_array_s(word_cnt_s) xor PADD_01_KEY;
                n_xoodoo_state_array_s(STATE_WORDS_C-1) <= xoodoo_state_array_s(STATE_WORDS_C-1) xor domain_s;
                

            when PRE_ABSORB_NONCE =>
                n_xoodoo_start_s <= '1';
                n_domain_s <= DOMAIN_ABSORB;
                

            when RUN_XOODOO =>
                n_xoodoo_start_s <= '0';
                if(xoodoo_valid_s = '1' and xoodoo_start_s = '0') then
                    n_xoodoo_state_array_s <= xoodoo_array_out_s;
                    --phase <= PHASE_UP;
                    -- if(next_state_s = EXTRACT_TAG) then
                        -- tag_ready_s <= '1';
                    -- end if;
                end if;
                

            --when SAMPLE_XOODOO_OUT =>
            --    n_xoodoo_state_array_s <= xoodoo_array_out_s;

            -- Store bdi_eoi (will only be effective on last word) and decrypt_in flag.
            when ABSORB_NONCE =>
                bdi_ready_s     <= '1';
                n_eoi_s         <= bdi_eoi;
                n_decrypt_s     <= decrypt_in;
                if (bdi_valid = '1' and bdi_ready_s = '1' and bdi_type = HDR_NPUB) then
                    n_xoodoo_state_array_s(word_cnt_s) <= xoodoo_state_array_s(word_cnt_s) xor bdi_s;
                    n_eoi_s     <= bdi_eoi;
                end if;


            when PADD_NONCE =>
                n_xoodoo_state_array_s(word_cnt_s) <= xoodoo_state_array_s(word_cnt_s) xor PADD_01;
                n_xoodoo_state_array_s(STATE_WORDS_C-1) <= xoodoo_state_array_s(STATE_WORDS_C-1) xor domain_s;
                

            when PRE_ABSORB_AD =>
                if not(bdi_valid = '1' and (bdi_type = HDR_PT or bdi_type = HDR_CT)) then
                    n_xoodoo_start_s <= '1';
                end if;
                

            -- If pt or ct is detected, don't assert bdi_ready, otherwise first word
            -- gets lost. Store bdi_eoi.
            -- padd() returns vector containing bdi_s and inserted 0x01 byte according
            -- to 0100* padding.
            when ABSORB_AD =>
                if not (bdi_valid = '1' and (bdi_type = HDR_PT or bdi_type = HDR_CT)) then
                    bdi_ready_s <= '1';
                end if;
                if (bdi_valid = '1' and bdi_ready_s = '1') then
                    n_eoi_s         <= bdi_eoi;
                    if (bdi_type = HDR_AD) then
                        n_xoodoo_state_array_s(word_cnt_s) <= xoodoo_state_array_s(word_cnt_s) xor padd(bdi_s, bdi_valid_bytes_s, bdi_pad_loc_s);
                    end if;
                end if;
                

            -- Set bdi_ready and connect the valid hash_msg bytes to tag_ram for
            -- absorbtion.
            when ABSORB_HASH_MSG =>
                bdi_ready_s <= '1';
                --if (bdi_valid = '1' and bdi_ready_s = '1' and bdi_type = HDR_HASH_MSG) then
                    --data_to_tag_s   <= padd(bdi_s, bdi_valid_bytes_s, bdi_pad_loc_s);
                    --tag_wen_s       <= '1';
                --end if;
                

            -- Insert 0x01 padding byte (state is only reached if not yet inserted).
            when PADD_AD | PADD_HASH_MSG =>
                if (word_cnt_s < STATE_WORDS_C-1 ) then
                    n_xoodoo_state_array_s(word_cnt_s) <= xoodoo_state_array_s(word_cnt_s) xor PADD_01;
                    n_xoodoo_state_array_s(STATE_WORDS_C-1) <= xoodoo_state_array_s(STATE_WORDS_C-1) xor domain_s;
                else
                    n_xoodoo_state_array_s(STATE_WORDS_C-1) <= xoodoo_state_array_s(STATE_WORDS_C-1) xor PADD_01 xor domain_s;
                end if;
                    n_domain_s <= DOMAIN_ZERO;
                    

            when PADD_AD_ONLY_DOMAIN =>
                n_xoodoo_state_array_s(STATE_WORDS_C-1) <= xoodoo_state_array_s(STATE_WORDS_C-1) xor domain_s;
                n_domain_s <= DOMAIN_ZERO;
                

            when PADD_AD_BLOCK =>
                       n_xoodoo_state_array_s(STATE_WORDS_C-1) <= xoodoo_state_array_s(STATE_WORDS_C-1) xor PADD_01 xor domain_s;
                       n_domain_s <= DOMAIN_ZERO;
                       

            when PRE_ABSORB_MSG =>
                bdi_ready_s     <= '0';
                if (first_block_s = '1') then
                    n_xoodoo_state_array_s(STATE_WORDS_C-1) <= xoodoo_state_array_s(STATE_WORDS_C-1) xor DOMAIN_CRYPT;
                    n_first_block_s <= '0';
                else
                    n_xoodoo_state_array_s(STATE_WORDS_C-1) <= xoodoo_state_array_s(STATE_WORDS_C-1) xor DOMAIN_ZERO;
                end if;
                n_xoodoo_start_s <= '1';
                
                
            -- Generate CT / PT
            when ABSORB_MSG =>
                bdi_ready_s <= bdo_ready;
                if (bdi_valid = '1' and bdi_ready_s = '1') then
                    if (decrypt_s = '0' and bdi_type = HDR_PT) then
                        n_xoodoo_state_array_s(word_cnt_s) <= xoodoo_state_array_s(word_cnt_s) xor padd(bdi_s, bdi_valid_bytes_s, bdi_pad_loc_s);

                    elsif(bdi_type = HDR_CT) then
                        n_xoodoo_state_array_s(word_cnt_s) <= (xoodoo_state_array_s(word_cnt_s) and select_bytes(bdi_s, bdi_valid_bytes_s)) xor padd(bdi_s, bdi_valid_bytes_s, bdi_pad_loc_s);
                    end if;
                end if;
                

            when PADD_MSG =>
                n_xoodoo_state_array_s(word_cnt_s) <= xoodoo_state_array_s(word_cnt_s) xor PADD_01;


            when PADD_MSG_ONLY_DOMAIN =>


            when PADD_MSG_BLOCK =>
                 n_xoodoo_state_array_s(word_cnt_s) <= xoodoo_state_array_s(word_cnt_s) xor PADD_01;


            when PRE_EXTRACT_TAG =>
                n_xoodoo_state_array_s(STATE_WORDS_C-1) <= xoodoo_state_array_s(STATE_WORDS_C-1) xor DOMAIN_SQUEEZE;
                n_xoodoo_start_s <= '1';

            
            when EXTRACT_TAG =>
                tag_ready_s <= '1';

            -- As soon as bdi input doesn't match with calculated tag in ram,
            -- reset msg_auth.
            when VERIFY_TAG =>
                bdi_ready_s <= '1';
                if (bdi_valid = '1' and bdi_ready_s = '1' and bdi_type = HDR_TAG) then
                    --if (bdi_s /= tag_dout_s) then
                    if (bdi_s /= xoodoo_state_array_s(word_cnt_s)) then
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
    p_counters : process(clk,rst)
    begin
        if (rst = '1') then
            word_cnt_s      <= 0;
        elsif rising_edge(clk) then
        
        -- if rising_edge(clk) then
            -- if (rst = '1') then
                -- word_cnt_s      <= 0;    
            -- else            
            

                --block_cnt_s     <= to_unsigned(1, block_cnt_s'length);
                --ad_byte_cnt_s   <= (others => '0');
                --msg_byte_cnt_s  <= (others => '0');
            --else
                case state_s is
                    -- Nothing to do here, reset counters
                    when IDLE =>
                       word_cnt_s      <= 0;
                        --block_cnt_s     <= to_unsigned(1, block_cnt_s'length);
                        --ad_byte_cnt_s   <= (others => '0');
                        --msg_byte_cnt_s  <= (others => '0');

                    -- If key is to be updated, increase counter on every successful
                    -- data transfer (valid and ready).
                    when STORE_KEY =>
                        if (key_update = '1') then
                            if (key_valid = '1' and key_ready_s = '1') then
                                --if (word_cnt_s >= KEY_WORDS_C - 1) then
                                if (word_cnt_s > KEY_WORDS_C - 1) then
                                    word_cnt_s <= 0;
                                else
                                    word_cnt_s <= word_cnt_s + 1;
                                end if;
                            end if;
                        else
                            --if (word_cnt_s >= KEY_WORDS_C - 1) then
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
                            --if (word_cnt_s >= NPUB_WORDS_C - 1) then
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
                    -- Additionally count number of ad bytes.
                    when ABSORB_AD =>
                        if (bdi_valid = '1' and bdi_ready_s = '1') then
                            --if (word_cnt_s >= BLOCK_WORDS_C - 1 or (bdi_eot = '1' and bdi_partial_s = '1')) then
                            if (word_cnt_s > RKIN_WORDS_C - 1 or (bdi_eot = '1' and bdi_partial_s = '1')) then
                                word_cnt_s <= 0;
                            else
                                word_cnt_s <= word_cnt_s + 1;
                            end if;
                            --ad_byte_cnt_s <= ad_byte_cnt_s + unsigned(bdi_size);
                        end if;

                    -- Increase word counter when transferring data.
                    -- Reset word counter if block size is reached or the last msg word is received
                    -- and only partially filled such that the 0x01 padding byte could be inserted.
                    -- Additionally count number of msg bytes.
                    when ABSORB_MSG =>
                        if (bdi_valid = '1' and bdi_ready_s = '1') then
                            --if (word_cnt_s >= BLOCK_WORDS_C - 1 or (bdi_eot = '1' and bdi_partial_s = '1')) then
                            if (word_cnt_s > RKOUT_WORDS_C - 1 or (bdi_eot = '1' and bdi_partial_s = '1')) then
                                word_cnt_s  <= 0;
                                --block_cnt_s <= block_cnt_s + 1;
                            else
                                word_cnt_s <= word_cnt_s + 1;
                            end if;
                            --msg_byte_cnt_s <= msg_byte_cnt_s + unsigned(bdi_size);
                        end if;

                    -- Increase word counter when transferring data until either the block size
                    -- for hash msg is reached or the last word is transferred and it's only
                    -- partially filled (again such that 0x01 can be inserted).
                    when ABSORB_HASH_MSG =>
                        if (bdi_valid = '1' and bdi_ready_s = '1') then
                            if (word_cnt_s >= HASH_WORDS_C - 1 or (bdi_eot = '1' and bdi_partial_s = '1')) then
                                word_cnt_s <= 0;
                            else
                                word_cnt_s <= word_cnt_s + 1;
                            end if;
                        end if;

                    -- Reset word counters here. Due to 0100* padding, only one 0x01
                    -- Byte has to be inserted in this state.
                    when PADD_AD | PADD_MSG | PADD_HASH_MSG | PADD_AD_BLOCK | PADD_AD_ONLY_DOMAIN | PADD_MSG_BLOCK | PADD_MSG_ONLY_DOMAIN  =>
                        word_cnt_s <= 0;

                    -- Increase word counter up to block size or hash block size
                    -- depending on whether input is currently hashed or not.
                    -- when ABSORB_LENGTH | INIT_HASH =>
                    when INIT_HASH =>
                        if (word_cnt_s >= BLOCK_WORDS_C - 1 and hash_s = '0')
                        or (word_cnt_s >= HASH_WORDS_C - 1 and hash_s = '1') then
                            word_cnt_s  <= 0;
                        else
                            word_cnt_s <= word_cnt_s + 1;
                        end if;

                    -- Increase word counter on valid bdo transfer until either
                    -- block size or hash block size is reached depending on whether
                    -- input is hashed or not.
                    when EXTRACT_TAG | EXTRACT_HASH_VALUE =>
                        --if (bdo_valid_s = '1' and bdo_ready = '1') then
                        if (tag_ready_s = '1' and bdo_ready = '1') then
                            if (word_cnt_s >= TAG_WORDS_C - 1 and hash_s = '0')
                            or (word_cnt_s >= HASH_WORDS_C - 1 and hash_s = '1') then
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
           -- end if;
        end if;
    end process p_counters;
    -- Cast the word counter to std_logic_vector for ram connection.
    -- ram_addr_s <= std_logic_vector(to_unsigned(word_cnt_s, ADDR_BITS_256_C));

    -- Resize the unsigned counters to 61 Bit and prepend three bit null vector to convert bytes to bit.
    --ad_bit_cnt_vec_s <= std_logic_vector(resize(ad_byte_cnt_s, ad_bit_cnt_vec_s'length - 3)) & "000";
    --msg_bit_cnt_vec_s <= std_logic_vector(resize(msg_byte_cnt_s, msg_bit_cnt_vec_s'length - 3)) & "000";

    -- Concatenate the lengths (in bits) of ad and pt/ct for tag absorbtion. Reorder due to Endianess.
    -- Extract single word from len_s vector.
    -- len_s <= reverse_byte(msg_bit_cnt_vec_s) & reverse_byte(ad_bit_cnt_vec_s);
    -- len_word_s <= len_s(CCW*(word_cnt_s+1) - 1 downto CCW*word_cnt_s) when (state_s = ABSORB_LENGTH) else (others => '-');

    -- Convert the block counter to a std_logic vector. Reorder due to Endianess.
    -- and extract single word from block_num_s vector.
    -- block_num_s <= reverse_byte(std_logic_vector(resize(block_cnt_s, block_num_s'length)));
    -- block_num_word_s <= block_num_s(CCW*(word_cnt_s+1) - 1 downto CCW*word_cnt_s) when (state_s = ABSORB_MSG) else (others => '-');

end behavioral;
