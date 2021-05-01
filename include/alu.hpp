#pragma once
#include <iostream>
#include <limits.h>   // for CHAR_BIT
#include "cpu.hpp"
namespace ALU {
	uint32_t rotr32 (uint32_t n, unsigned int c);
	uint8_t aluCarry();
	uint8_t aluOverflow();
	uint32_t ADD(uint64_t a, uint64_t b);

	uint8_t shiftCarry();
	uint32_t shifter(uint32_t in, uint32_t c, uint32_t type);

}