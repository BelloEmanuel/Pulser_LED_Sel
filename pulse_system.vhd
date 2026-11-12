library ieee;
use ieee.std_logic_1164.all;

entity pulse_system is
    port (
        clk         : in  std_logic;
        uart_rx     : in  std_logic;
        uart_tx     : out std_logic;

        pulse_out   : out std_logic;
        trigger_out : out std_logic;

        led_sel     : out std_logic_vector(1 downto 0)
    );
end entity pulse_system;


architecture rtl of pulse_system is

    -- USB-UART receiver updates rx_byte and allows for update of selected LED
    signal rx_byte       : std_logic_vector(7 downto 0);
    signal byte_received : std_logic;
    signal pulse_enable  : std_logic := '0';

    type ack_state_type is (ACK_IDLE, ACK_SEND, ACK_WAIT_BUSY, ACK_WAIT_DONE);
    signal ack_state    : ack_state_type := ACK_IDLE;
    signal ack_index    : integer range 0 to 5 := 0;
    signal ack_command  : std_logic_vector(7 downto 0) := (others => '0');
    signal ack_tx_data  : std_logic_vector(7 downto 0);
    signal ack_tx_start : std_logic := '0';
    signal ack_tx_busy  : std_logic;

begin
    -- create instance of uart-receiver
    UART_INST : entity work.uart_receiver
        generic map (
            CLK_FREQ  => 100_000_000,
            BAUD_RATE => 115_200
        )
        port map (
            clk           => clk,
            uart_rx       => uart_rx,
            rx_byte       => rx_byte,
            byte_received => byte_received
        );

    UART_TX_INST : entity work.uart_transmitter
        generic map (
            CLK_FREQ  => 100_000_000,
            BAUD_RATE => 115_200
        )
        port map (
            clk      => clk,
            tx_data  => ack_tx_data,
            tx_start => ack_tx_start,
            uart_tx  => uart_tx,
            tx_busy  => ack_tx_busy
        );


    -- create instance of the led-selector
    LED_INST : entity work.led_selector
        port map (
            clk           => clk,
            byte_received => byte_received,
            rx_byte       => rx_byte,
            led_sel       => led_sel
        );


    -- UART commands: 's' starts the pulsing and 't' stops it
    process(clk)
    begin
        if rising_edge(clk) then
            if byte_received = '1' then
                case rx_byte is
                    when x"73" => pulse_enable <= '1'; -- ASCII 's'
                    when x"74" => pulse_enable <= '0'; -- ASCII 't'
                    when others => null;
                end case;
            end if;
        end if;
    end process;

    -- Send "ACK <command>\n" back to computer after a valid command was accepted
    with ack_index select ack_tx_data <=
        x"41"       when 0, -- A
        x"43"       when 1, -- C
        x"4B"       when 2, -- K
        x"20"       when 3, -- space
        ack_command when 4,
        x"0A"       when others; -- new line

    process(clk)
    begin
        if rising_edge(clk) then
            ack_tx_start <= '0';

            case ack_state is
                when ACK_IDLE =>
                    if byte_received = '1' then
                        case rx_byte is
                            when x"31" | x"32" | x"33" | x"72" | x"73" | x"74" =>
                                ack_command <= rx_byte;
                                ack_index   <= 0;
                                ack_state   <= ACK_SEND;
                            when others => null;
                        end case;
                    end if;

                when ACK_SEND =>
                    ack_tx_start <= '1';
                    ack_state    <= ACK_WAIT_BUSY;

                when ACK_WAIT_BUSY =>
                    if ack_tx_busy = '1' then
                        ack_state <= ACK_WAIT_DONE;
                    end if;

                when ACK_WAIT_DONE =>
                    if ack_tx_busy = '0' then
                        if ack_index = 5 then
                            ack_state <= ACK_IDLE;
                        else
                            ack_index <= ack_index + 1;
                            ack_state <= ACK_SEND;
                        end if;
                    end if;
            end case;
        end if;
    end process;

    -- Generate the pulse train when enabled.
    PULSE_INST : entity work.pulse_generator
        port map (
            clk         => clk,
            enable      => pulse_enable,
            pulse_out   => pulse_out,
            trigger_out => trigger_out
        );

end architecture rtl;
