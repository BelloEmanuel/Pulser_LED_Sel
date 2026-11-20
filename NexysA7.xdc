set_property -dict {PACKAGE_PIN E3 IOSTANDARD LVCMOS33} [get_ports clk];
create_clock -add -name sys_clk_pin -period 10.00 -waveform {0 5} [get_ports clk];

# Pulse signal out, Pin 10 JXADC
set_property -dict {PACKAGE_PIN A18 IOSTANDARD LVCMOS33 DRIVE 16 SLEW FAST} [get_ports pulse_out];

# Trigger signal out, Pin 4 JA
set_property -dict {PACKAGE_PIN G17 IOSTANDARD LVCMOS33 DRIVE 16 SLEW FAST} [get_ports trigger_out];

# USB-UART: data transmitted by the computer to the FPGA
set_property -dict {PACKAGE_PIN C4 IOSTANDARD LVCMOS33} [get_ports uart_rx];

# USB-UART: acknowledgement transmitted by the FPGA to the computer
set_property -dict {PACKAGE_PIN D4 IOSTANDARD LVCMOS33} [get_ports uart_tx];

# Switch-IC control signals V1/V2, Pins 3 and 4 JXADC
set_property -dict {PACKAGE_PIN B18 IOSTANDARD LVCMOS33} [get_ports {led_sel[0]}];
set_property -dict {PACKAGE_PIN B16 IOSTANDARD LVCMOS33} [get_ports {led_sel[1]}];
