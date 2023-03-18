#include "dmg.h"

#include <thread>
// #include <fstream>
#include <cstdio>

uint8_t* Dmg::s_debug_mem = nullptr;

Dmg::Dmg()
	: m_bus()
	, m_mem(m_bus)
	, m_cpu(m_bus, m_mem)
	, m_is_powered_on(false)
{
	// very ugly
	s_debug_mem = m_mem.direct_ram();
}

void Dmg::insert_cartridge(std::string const& path)
{
	// std::ifstream cart(path, std::ios::binary);
	FILE* cart = fopen(path.c_str(), "rb");
	if (!cart)
	{
		printf("Could not open file '%s' for reading.\n", path.c_str());
		return;
	}

	int c;
	uint16_t i = 0xffff;

	while ((c = fgetc(cart)) != EOF && ++i != 0xffff)
	{
	 	// printf("$%04x: %02X\n", i, static_cast<uint8_t>(c));
		m_mem.direct_ram()[i] = static_cast<uint8_t>(c);
	}
	
	// int c;
	// uint16_t i = 0xffff;
	// uint8_t old_c = 0;

	// while ((c = fgetc(cart)) != EOF && ++i != 0xffff)
	// {
	// 	if (i % 2 == 0)
	// 		old_c = static_cast<uint8_t>(c);
	// 	else
	// 	{
	// 		m_mem.direct_ram()[i - 1] = static_cast<uint8_t>(c);
	// 		m_mem.direct_ram()[i] = old_c;
	// 		printf("$%04x: %02X\n", i - 1, static_cast<uint8_t>(c));
	// 		printf("$%04x: %02X\n", i, old_c);
	// 	}
	// }

	fclose(cart);
}

void Dmg::power_on()
{
	m_is_powered_on = true;

	while (m_is_powered_on)
	{
		m_cpu.clock();
		m_mem.clock();

		// using namespace std::chrono_literals;
		// std::this_thread::sleep_for(100ms);
	}
}

void Dmg::power_off()
{
	m_is_powered_on = false;
}