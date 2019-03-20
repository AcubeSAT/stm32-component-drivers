#ifndef MCP9808DRIVER_MCP9808_CONSTANTS_HPP
#define MCP9808DRIVER_MCP9808_CONSTANTS_HPP

// Master address (manufacturer-defined, used on the I2C bus)
#define MCP9808_I2C_BASE_ADDR                 (0x30)

// User defined address (bits 3-1, MSB left, not for usage in code - use the last 3 bits as a setting)
#define MCP9808_I2C_USER_BITS                 (0x00)

// User defined address (for usage in read-write requests)
#define MCP9808_I2C_USER_ADDR                 (static_cast<uint16_t>(MCP9808_I2C_BASE_ADDR | MCP9808_I2C_USER_BITS))


// Register addresses
#define MCP9808_REG_RFU                       (0x00u)
#define MCP9808_REG_CONFIG                    (0x01u)
#define MCP9808_REG_TUPPER                    (0x02u)
#define MCP9808_REG_TLOWER                    (0x03u)
#define MCP9808_REG_TCRIT                     (0x04u)
#define MCP9808_REG_TEMP                      (0x05u)
#define MCP9808_REG_MFGID                     (0x06u)
#define MCP9808_REG_DEVID                     (0x07u)
#define MCP9808_REG_RESOLUTION                (0x08u)

// Configuration masks (use these only for any configuration
// operations so that you don't overwrite critical data)

#define MCP9808_CONFIG_THYST_0C               (0x000U)
#define MCP9808_CONFIG_THYST_1_5C             (0x200U)
#define MCP9808_CONFIG_THYST_3C               (0x400U)
#define MCP9808_CONFIG_THYST_6C               (0x600U)




#endif //MCP9808DRIVER_MCP9808_CONSTANTS_HPP
