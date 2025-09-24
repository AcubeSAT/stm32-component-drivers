#pragma once

#include <etl/to_string.h>
#include <etl/String.hpp>

etl::array<uint8_t, 8192> nandTestData = {};
etl::array<uint8_t, 8192> nandFullPageReadBuffer{};
etl::array<uint8_t, 335> nandECCTestBuffer{};
