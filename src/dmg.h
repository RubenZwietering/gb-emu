#pragma once
#include <cstdint>
#include <string>

#include "bus.h"
#include "mem.h"
#include "cpu.h"

class Dmg
{
public:

	Dmg();
	~Dmg() = default;

	void insert_cartridge(std::string const& path);

	void power_on();
	void power_off();

private:
	Bus m_bus;
	Mem m_mem;
	Cpu m_cpu;

	bool m_is_powered_on;

};