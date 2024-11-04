#pragma once

enum class I2CError {
    None,           // No error
    WriteRegError,
    WriteError,
    HALError,       // HAL error occurred
    InvalidParams,  // Invalid input parameters
    Timeout         // Operation timed out
};


