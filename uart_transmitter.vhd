library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-- UART transmitter for feedback communication after receiving a signal from the computer
entity uart_transmitter is
    generic (
        CLK_FREQ  : integer := 100_000_000;
        BAUD_RATE : integer := 115_200
    );
    port (
        clk      : in  std_logic;
        tx_data  : in  std_logic_vector(7 downto 0);
        tx_start : in  std_logic;
        uart_tx  : out std_logic;
        tx_busy  : out std_logic
    );
end entity uart_transmitter;

architecture rtl of uart_transmitter is
    constant CLKS_PER_BIT : integer := CLK_FREQ / BAUD_RATE;

    type state_type is (IDLE, START_BIT, DATA_BITS, STOP_BIT);
    signal state       : state_type := IDLE;
    signal clock_count : integer range 0 to CLKS_PER_BIT - 1 := 0;
    signal bit_index   : integer range 0 to 7 := 0;
    signal tx_data_reg : std_logic_vector(7 downto 0) := (others => '0');
    signal tx_reg      : std_logic := '1';
    signal busy_reg    : std_logic := '0';

-- same working principle as the receiver just without the synchronization in the first stage
-- tx_data_reg consecutively stores the bytes that will be transmitted
begin
    process(clk)
    begin
        if rising_edge(clk) then
            case state is
                when IDLE =>
                    tx_reg   <= '1';
                    busy_reg <= '0';
                    if tx_start = '1' then
                        tx_data_reg <= tx_data;
                        tx_reg      <= '0'; -- Start bit
                        busy_reg    <= '1';
                        clock_count <= CLKS_PER_BIT - 1;
                        state       <= START_BIT;
                    end if;

                when START_BIT =>
                    if clock_count = 0 then
                        tx_reg      <= tx_data_reg(0);
                        bit_index   <= 0;
                        clock_count <= CLKS_PER_BIT - 1;
                        state       <= DATA_BITS;
                    else
                        clock_count <= clock_count - 1;
                    end if;

                when DATA_BITS =>
                    if clock_count = 0 then
                        clock_count <= CLKS_PER_BIT - 1;
                        if bit_index = 7 then
                            tx_reg <= '1'; -- Stop bit
                            state  <= STOP_BIT;
                        else
                            bit_index <= bit_index + 1;
                            tx_reg    <= tx_data_reg(bit_index + 1);
                        end if;
                    else
                        clock_count <= clock_count - 1;
                    end if;

                when STOP_BIT =>
                    if clock_count = 0 then
                        tx_reg   <= '1';
                        busy_reg <= '0';
                        state    <= IDLE;
                    else
                        clock_count <= clock_count - 1;
                    end if;
            end case;
        end if;
    end process;

    uart_tx <= tx_reg;
    tx_busy <= busy_reg;
end architecture rtl;
