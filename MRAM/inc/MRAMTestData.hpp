//
// Created by tats on 21/11/2024.
//

#pragma once
#include <etl/to_string.h>
#include <etl/String.hpp>

etl::array<uint8_t, 200> mram_test_data = {
    76, 111, 114, 101, 109, 32, 105, 112, 115, 117,
    109, 32, 100, 111, 108, 111, 114, 32, 115, 105,
    116, 32, 97, 109, 101, 116, 44, 32, 99, 111,
    110, 115, 101, 99, 116, 101, 116, 117, 114, 32,
    97, 100, 105, 112, 105, 115, 99, 105, 110, 103,
    32, 101, 108, 105, 116, 46, 32, 65, 101, 110,
    101, 97, 110, 32, 118, 105, 118, 101, 114, 114,
    97, 44, 32, 110, 105, 98, 104, 32, 118, 101,
    108,32, 99, 111, 110, 115, 101, 113, 117, 97,
    116, 32, 115, 117, 115, 99, 105, 112, 105, 116,
    44, 32, 101, 120, 32, 110, 105, 115, 108, 32,
    102, 101, 114, 109, 101, 110, 116, 117, 109, 32,
    110,105, 115, 105, 44, 32, 101, 103, 101, 116,
    32, 108, 117, 99, 116, 117, 115, 32, 102, 101,
    108, 105, 115, 32, 112, 117, 114, 117, 115, 32,
    110, 111, 110, 32, 109, 97, 117, 114, 105, 115,
    46,32, 68, 111, 110, 101, 99, 32, 110, 117,
    110, 99, 32, 116, 111, 114, 116, 111, 114, 44,
    32, 98, 108, 97, 110, 100, 105, 116, 32, 115,
    101, 100, 32, 115, 117, 115, 99, 105, 112, 105};

//etl::array<uint8_t, 200> mram_test_data = {
//    1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
//    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
//    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
//    31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
//    41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
//    51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
//    61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
//    71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
//    81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
//    91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
//    101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
//    111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
//    121, 122, 123, 124, 125, 126, 127, 128, 129, 130,
//    131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
//    141, 142, 143, 144, 145, 146, 147, 148, 149, 150,
//    151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
//    161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
//    171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
//    181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
//    191, 192, 193, 194, 195, 196, 197, 198, 199, 200
//};