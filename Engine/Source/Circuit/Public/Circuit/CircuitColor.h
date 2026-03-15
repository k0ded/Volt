#pragma once
#include "Circuit/Config.h"
#include <cstdint>
class CircuitColor
{
public:
	union
	{
		uint32_t m_Hex;
		struct { uint8_t m_A, m_B, m_G, m_R; };
	};

	CIRCUIT_API CircuitColor(uint32_t hex = 0xFFFFFFFF);
	CIRCUIT_API CircuitColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

	CIRCUIT_API void operator=(const uint32_t& hex);

};
