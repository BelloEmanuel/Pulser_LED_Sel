library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity pulse_generator is
    port (
        clk         : in  std_logic;
        enable      : in  std_logic;
        pulse_out   : out std_logic;
        trigger_out : out std_logic
    );
end entity;

architecture rtl of pulse_generator is

    constant PERIOD       : natural := 100000;  -- 1 ms for 100 MHz frequency
    constant PULSE_CYCLES : natural := 3;       -- times 10ns for on duration

    signal counter : unsigned(16 downto 0) := (others => '0');
    signal pulse_r : std_logic := '0';
    signal trigger_r : std_logic := '0';

    attribute IOB : string;
    attribute IOB of pulse_r : signal is "TRUE";
    attribute IOB of trigger_r : signal is "TRUE";

begin

    process(clk)
    begin
        if rising_edge(clk) then

            -- Disabling returns both outputs (pulse and trigger) LOW and restarts the period
            if enable = '0' then
                counter   <= (others => '0');
                pulse_r <= '0';
                trigger_r <= '0';
            else
                -- Counter
                if counter = to_unsigned(PERIOD - 1, counter'length) then
                    counter <= (others => '0');
                else
                    counter <= counter + 1;
                end if;

                -- Change output register
                if counter = to_unsigned(PERIOD - PULSE_CYCLES - 1, counter'length) then
                    pulse_r <= '1';
                    trigger_r <= '1';
                elsif counter = to_unsigned(PERIOD - 1, counter'length) then
                    pulse_r <= '0';
                    trigger_r <= '0';
                end if;
            end if;

        end if;
    end process;

    pulse_out <= pulse_r;
    trigger_out <= trigger_r;

end architecture;
