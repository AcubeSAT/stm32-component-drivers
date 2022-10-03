# ATSAMV71Q21 Component Drivers

Libraries for interfacing various hardware components and ICs with ATSAM microcontrollers

# Connecting the MCP9808 at the Xplained V71 Dev Board

## Pins 

Data CPU Pin PD27 is assigned to D20 on the Dev board

Clock CPU Pin PD28 is assigned to D21 on the Dev board

Use a pair of Pull-Up Resistors (1kΩ) each, one from Data and one from Clock, up to VCC

MCP9808 Connections:
| Pin Number | Connection | Notes |
| ------ | ----- | ----- |
| 1 | Data | Use a pull up resistor to 3.3V |
| 2 | Clock | Use a pull up resistor to 3.3V |
| 3 | Alert | AFAIK We don't use this? |
| 4 | Ground | Set to one of the dev board's grounds |
| 5 | A2 | Set to ground |
| 6 | A1 | Set to ground |
| 7 | A0 | Set to ground |
| 8 | Vdd | Set to 3V3 for the dev board |

Our breakout board currently skips two pins Pin 4 and Pin 7. For example MCP's Pin 4 is connected to the breakout's Pin 5. 
