library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-- Nexys A7 has 100MHz clock, we use 115200 BAUD for the signal transmission
-- uart_rx receives the data from USB-UART interface
-- rx_byte contains the last received complete byte
entity uart_receiver is
    generic (
        CLK_FREQ  : integer := 100_000_000;
        BAUD_RATE : integer := 115_200
    );
    port (
        clk           : in  std_logic;
        uart_rx       : in  std_logic;
        rx_byte       : out std_logic_vector(7 downto 0);
        byte_received : out std_logic
    );
end entity uart_receiver;

architecture rtl of uart_receiver is
    constant CLKS_PER_BIT : integer := CLK_FREQ / BAUD_RATE;
    -- 4 states for the receiver machine to operate in, at the beginning it is IDLE
    type state_type is (IDLE, START_BIT, DATA_BITS, STOP_BIT);
    signal state : state_type := IDLE;
    -- Synchronize the asynchron UART-signal to increase stability
    signal rx_sync_1 : std_logic := '1';
    signal rx_sync_2 : std_logic := '1';
    -- clk counts to wait until next uart bit is to read
    signal clock_count : integer range 0 to CLKS_PER_BIT - 1 := 0;
    signal bit_index   : integer range 0 to 7 := 0;
    -- stores the 8 data bits while receiving
    signal rx_data     : std_logic_vector(7 downto 0) := (others => '0');
    -- stores the last fully received byte
    signal rx_byte_reg : std_logic_vector(7 downto 0) := (others => '0');
    -- indicate that a full byte has been received, one clk cycle
    signal byte_received_reg : std_logic := '0';

begin
    -- synchronize input on clk
    process(clk)
    begin
        if rising_edge(clk) then
            rx_sync_1 <= uart_rx;
            rx_sync_2 <= rx_sync_1;
        end if;
    end process;

    -- UART-Empfang
    process(clk)
    begin
        if rising_edge(clk) then
            -- by default we have no full byte received
            byte_received_reg <= '0';
            case state is
                -- when IDLE the receiver waits for the beginning of a transmission
                -- by default UART is HIGH so LOW indicates a possible start bit
                when IDLE =>
                    clock_count <= 0;
                    bit_index   <= 0;
                    if rx_sync_2 = '0' then
                        clock_count <= CLKS_PER_BIT / 2;
                        state       <= START_BIT;
                    end if;
                -- after receiving the start bit we test the middle position of the start bit to verify it
                when START_BIT =>
                    if clock_count = 0 then
                        -- if the middle of the start bit is still 0 we start with recording data, otherwise it was a false start
                        if rx_sync_2 = '0' then
                            clock_count <= CLKS_PER_BIT - 1;
                            bit_index   <= 0;
                            state       <= DATA_BITS;
                        else
                            state <= IDLE;
                        end if;
                    else
                        clock_count <= clock_count - 1;
                    end if;
                -- now we know that we have a signal transmission, so we wait one cycle and then read in the middle, this we do for every bit of the signal
                when DATA_BITS =>
                    if clock_count = 0 then
                        -- data bits are transmitted LSB via UART
                        rx_data(bit_index) <= rx_sync_2;
                        clock_count <= CLKS_PER_BIT - 1;
                        if bit_index = 7 then
                            state <= STOP_BIT;
                        else
                            bit_index <= bit_index + 1;
                        end if;
                    else
                        clock_count <= clock_count - 1;
                    end if;
                -- the stop bit is HIGH for UART 8N1, so we wait again for one cycle to then check
                when STOP_BIT =>
                    if clock_count = 0 then
                        -- if the stop bit is valid, meaning HIGH, we actually store the transmitted byte and marc that it has been received, then we wait again for the next transmission
                        if rx_sync_2 = '1' then
                            rx_byte_reg      <= rx_data;
                            byte_received_reg <= '1';
                        end if;
                        state <= IDLE;
                    else
                        clock_count <= clock_count - 1;
                    end if;
            end case;
        end if;
    end process;
    rx_byte       <= rx_byte_reg;
    byte_received <= byte_received_reg;
end architecture rtl;