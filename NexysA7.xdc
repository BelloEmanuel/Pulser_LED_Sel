set_property -dict {PACKAGE_PIN E3 IOSTANDARD LVCMOS33} [get_ports clk];
create_clock -add -name sys_clk_pin -period 10.00 -waveform {0 5} [get_ports clk];
set_property -dict {PACKAGE_PIN A18 IOSTANDARD LVCMOS33 DRIVE 16 SLEW FAST} [get_ports pulse_out];
set_property -dict {PACKAGE_PIN G17 IOSTANDARD LVCMOS33 DRIVE 16 SLEW FAST} [get_ports trigger_out];

# USB-UART: data transmitted by the computer to the FPGA.
set_property -dict {PACKAGE_PIN C4 IOSTANDARD LVCMOS33} [get_ports uart_rx];
# USB-UART: acknowledgement transmitted by the FPGA to the computer.
set_property -dict {PACKAGE_PIN D4 IOSTANDARD LVCMOS33} [get_ports uart_tx];

# Pmod JXADC pins 1 and 2 provide the PE613050 V1/V2 control word.
# The JXADC header also carries pulse_out on pin 10.
set_property -dict {PACKAGE_PIN B18 IOSTANDARD LVCMOS33} [get_ports {led_sel[0]}];
set_property -dict {PACKAGE_PIN B16 IOSTANDARD LVCMOS33} [get_ports {led_sel[1]}];
