--------------------------------------------------------------------------------
--! @file       LWC.vhd (CAESAR API for Lightweight)
--! @brief      LWC top level file
--! @author     Panasayya Yalla & Ekawat (ice) Homsirikamol
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
--!
--!
--!
--!
--!
--!
--------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use work.Design_pkg.all;
use work.NIST_LWAPI_pkg.all;

entity LWC is
    port (
        --! Global ports
        clk             : in  std_logic;
        rst             : in  std_logic;
        --! Publica data ports
        pdi_data        : in  std_logic_vector(W-1 downto 0);
        pdi_valid       : in  std_logic;
        pdi_ready       : out std_logic;
        --! Secret data ports
        sdi_data        : in  std_logic_vector(SW-1 downto 0);
        sdi_valid       : in  std_logic;
        sdi_ready       : out std_logic;
        --! Data out ports
        do_data         : out std_logic_vector(W-1 downto 0);
        do_ready        : in  std_logic;
        do_valid        : out std_logic;
        do_last         : out std_logic
    );
end LWC;

architecture structure of LWC is
    --==========================================================================
    --!Cipher
    --==========================================================================
    ------!Pre-Processor to Cipher (Key PISO)
    signal key_cipher_in            : std_logic_vector(CCSW    -1 downto 0);
    signal key_valid_cipher_in      : std_logic;
    signal key_ready_cipher_in      : std_logic;
    ------!Pre-Processor to Cipher (DATA PISO)
    signal bdi_cipher_in            : std_logic_vector(CCW     -1 downto 0);
    signal bdi_valid_cipher_in      : std_logic;
    signal bdi_ready_cipher_in      : std_logic;
    --
    signal bdi_pad_loc_cipher_in    : std_logic_vector(CCWdiv8 -1 downto 0);
    signal bdi_valid_bytes_cipher_in: std_logic_vector(CCWdiv8 -1 downto 0);
    signal bdi_size_cipher_in       : std_logic_vector(3       -1 downto 0);
    signal bdi_eot_cipher_in        : std_logic;
    signal bdi_eoi_cipher_in        : std_logic;
    signal bdi_type_cipher_in       : std_logic_vector(4       -1 downto 0);
    signal decrypt_cipher_in        : std_logic;
    signal hash_cipher_in           : std_logic;
    signal key_update_cipher_in     : std_logic;
    ------!Cipher(DATA SIPO) to Post-Processor
    signal bdo_cipher_out           : std_logic_vector(CCW     -1 downto 0);
    signal bdo_valid_cipher_out     : std_logic;
    signal bdo_ready_cipher_out     : std_logic;
    ------!Cipher to Post-Processor
    signal end_of_block_cipher_out  : std_logic;
    signal bdo_size_cipher_out      : std_logic_vector(3       -1 downto 0);
    signal bdo_valid_bytes_cipher_out:std_logic_vector(CCWdiv8 -1 downto 0);
    signal bdo_type_cipher_out      :std_logic_vector(4        -1 downto 0);
    signal decrypt_cipher_out       : std_logic;
    signal msg_auth_valid           : std_logic;
    signal msg_auth_ready           : std_logic;
    signal msg_auth                 : std_logic;
    signal done                     : std_logic;
    --==========================================================================

    --==========================================================================
    --!FIFO
    --==========================================================================
    ------!Pre-Processor to FIFO
    signal cmd_FIFO_in              : std_logic_vector(W-1 downto 0);
    signal cmd_valid_FIFO_in        : std_logic;
    signal cmd_ready_FIFO_in        : std_logic;
    ------!FIFO to Post_Processor
    signal cmd_FIFO_out             : std_logic_vector(W-1 downto 0);
    signal cmd_valid_FIFO_out       : std_logic;
    signal cmd_ready_FIFO_out       : std_logic;
    --==========================================================================
begin

    assert (ASYNC_RSTN = false) report "Asynchronous reset is not supported!" severity failure;

    Inst_PreProcessor: entity work.PreProcessor(PreProcessor)
        PORT MAP(
                clk             => clk                                     ,
                rst             => rst                                     ,
                pdi_data        => pdi_data                                ,
                pdi_valid       => pdi_valid                               ,
                pdi_ready       => pdi_ready                               ,
                sdi_data        => sdi_data                                ,
                sdi_valid       => sdi_valid                               ,
                sdi_ready       => sdi_ready                               ,
                key             => key_cipher_in                           ,
                key_valid       => key_valid_cipher_in                     ,
                key_ready       => key_ready_cipher_in                     ,
                bdi             => bdi_cipher_in                           ,
                bdi_valid       => bdi_valid_cipher_in                     ,
                bdi_ready       => bdi_ready_cipher_in                     ,
                bdi_pad_loc     => bdi_pad_loc_cipher_in                   ,
                bdi_valid_bytes => bdi_valid_bytes_cipher_in               ,
                bdi_size        => bdi_size_cipher_in                      ,
                bdi_eot         => bdi_eot_cipher_in                       ,
                bdi_eoi         => bdi_eoi_cipher_in                       ,
                bdi_type        => bdi_type_cipher_in                      ,
                decrypt         => decrypt_cipher_in                       ,
                hash            => hash_cipher_in                          ,
                key_update      => key_update_cipher_in                    ,
                cmd             => cmd_FIFO_in                             ,
                cmd_valid       => cmd_valid_FIFO_in                       ,
                cmd_ready       => cmd_ready_FIFO_in
            );
    Inst_Cipher: entity work.CryptoCore(behavioral)
        PORT MAP(
                clk             => clk                                     ,
                rst             => rst                                     ,
                key             => key_cipher_in                           ,
                key_valid       => key_valid_cipher_in                     ,
                key_ready       => key_ready_cipher_in                     ,
                bdi             => bdi_cipher_in                           ,
                bdi_valid       => bdi_valid_cipher_in                     ,
                bdi_ready       => bdi_ready_cipher_in                     ,
                bdi_pad_loc     => bdi_pad_loc_cipher_in                   ,
                bdi_valid_bytes => bdi_valid_bytes_cipher_in               ,
                bdi_size        => bdi_size_cipher_in                      ,
                bdi_eot         => bdi_eot_cipher_in                       ,
                bdi_eoi         => bdi_eoi_cipher_in                       ,
                bdi_type        => bdi_type_cipher_in                      ,
                decrypt_in      => decrypt_cipher_in                       ,
                hash_in         => hash_cipher_in                          ,
                key_update      => key_update_cipher_in                    ,
                bdo             => bdo_cipher_out                          ,
                bdo_valid       => bdo_valid_cipher_out                    ,
                bdo_ready       => bdo_ready_cipher_out                    ,
                bdo_type        => bdo_type_cipher_out                     ,
                bdo_valid_bytes => bdo_valid_bytes_cipher_out              ,
                end_of_block    => end_of_block_cipher_out                 ,
                msg_auth_valid  => msg_auth_valid                          ,
                msg_auth_ready  => msg_auth_ready                          ,
                msg_auth        => msg_auth
            );
    Inst_PostProcessor: entity work.PostProcessor(PostProcessor)
        PORT MAP(
                clk             => clk                                     ,
                rst             => rst                                     ,
                bdo             => bdo_cipher_out                          ,
                bdo_valid       => bdo_valid_cipher_out                    ,
                bdo_ready       => bdo_ready_cipher_out                    ,
                end_of_block    => end_of_block_cipher_out                 ,
                bdo_type        => bdo_type_cipher_out                     ,
                bdo_valid_bytes => bdo_valid_bytes_cipher_out              ,
                cmd             => cmd_FIFO_out                            ,
                cmd_valid       => cmd_valid_FIFO_out                      ,
                cmd_ready       => cmd_ready_FIFO_out                      ,
                do_data         => do_data                                 ,
                do_valid        => do_valid                                ,
                do_last         => do_last                                 ,
                do_ready        => do_ready                                ,
                msg_auth_valid  => msg_auth_valid                          ,
                msg_auth_ready  => msg_auth_ready                          ,
                msg_auth        => msg_auth
            );
    Inst_Header_Fifo: entity work.fwft_fifo(structure)
        generic map (
                G_W             => W,
                G_LOG2DEPTH     => 2
            )
        PORT MAP(
                clk             => clk,
                rst             => rst,
                din             => cmd_FIFO_in,
                din_valid       => cmd_valid_FIFO_in,
                din_ready       => cmd_ready_FIFO_in,
                dout            => cmd_FIFO_out,
                dout_valid      => cmd_valid_FIFO_out,
                dout_ready      => cmd_ready_FIFO_out
            );



end structure;
