#pragma once
#include <cstdint>
#include <array>

#include "bus.h"

class Mem
{
public:

	Mem(Bus& bus);
	~Mem() = default;

	void clock();

	uint8_t* direct_ram() { return m_ram; }

private:
	Bus& m_bus;

	uint8_t m_ram[0xffff];
};