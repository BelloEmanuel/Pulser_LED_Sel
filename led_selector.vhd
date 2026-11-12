library ieee;
use ieee.std_logic_1164.all;

-- we expect the logic byte_received from uart_receiver.vhd, marking that we have to update the selected LED
-- the value to update to is stored in rx_byte, also provided by uard_receiver.vhd
entity led_selector is
    port (
        clk           : in  std_logic;
        byte_received : in  std_logic;
        rx_byte       : in  std_logic_vector(7 downto 0);

        led_sel       : out std_logic_vector(1 downto 0)
    );
end entity led_selector;

architecture rtl of led_selector is
    -- Two-bit control word for the PE613050 V1/V2 inputs
    -- Update these assignments to match the final PCB routing
    signal led_reg : std_logic_vector(1 downto 0) := "00";

begin

    process(clk)
    begin
        if rising_edge(clk) then
            if byte_received = '1' then
                case rx_byte is
                    when x"72" => led_reg <= "00"; -- reset ASII 'r' / Select LED 0
                    when x"31" => led_reg <= "10"; -- Select LED 1
                    when x"32" => led_reg <= "01"; -- Select LED 2
                    when x"33" => led_reg <= "11"; -- Select LED 3
                    when others => null;
                end case;
            end if;
        end if;
    end process;

    led_sel <= led_reg;

end architecture rtl;
