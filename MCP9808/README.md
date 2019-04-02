# Microchip Technology MCP9808 Temperature Sensor 
*Driver for STM32-based microcontrollers*

## Dependencies
Make sure to have the STM32 HAL functions for I2C available and enabled!

## How to use
Just include the repo as a submodule (or clone it) in your project and 
`#include "MCP9808.hpp"` wherever you need to.

For more details refer to the documentation (generate it via the provided Doxyfile or
just read the docstrings inside `MCP9808.hpp`, whatever floats your boat)

## File listing
* README.md - this readme file
##### `Inc/` folder
* MCP9808.hpp - header describing the API
* MCP9808-constants.hpp - constants file listing the available options
##### `Src/` folder
* MCP9808.cpp - implementation of the interface functions
* MCP9808-internal.cpp - STM32-specific I/O functions (for communication)

TODO: Test with negative temperatures!
TODO: Implement interrupt (alert) functionality (if needed)
