#ifndef MCP9808DRIVER_MCP9808_CONSTANTS_HPP
#define MCP9808DRIVER_MCP9808_CONSTANTS_HPP

// Master address (manufacturer-defined, used on the I2C bus)
#define MCP9808_I2C_BASE_ADDR                 (0x30)

// User defined address (bits 3-1, MSB left, not for usage in code - use the last 3 bits as a setting)
#define MCP9808_I2C_USER_BITS                 (0x00)

// User defined address (for usage in read-write requests)
#define MCP9808_I2C_USER_ADDR                 (static_cast<uint16_t>(MCP9808_I2C_BASE_ADDR | MCP9808_I2C_USER_BITS) >> 1)


// Register addresses
#define MCP9808_REG_RFU                       (0x00)
#define MCP9808_REG_CONFIG                    (0x01)
#define MCP9808_REG_TUPPER                    (0x02)
#define MCP9808_REG_TLOWER                    (0x03)
#define MCP9808_REG_TCRIT                     (0x04)
#define MCP9808_REG_TEMP                      (0x05)
#define MCP9808_REG_MFGID                     (0x06)
#define MCP9808_REG_DEVID                     (0x07)
#define MCP9808_REG_RESOLUTION                (0x08)

#endif //MCP9808DRIVER_MCP9808_CONSTANTS_HPP
