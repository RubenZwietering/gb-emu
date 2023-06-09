#include "mem.h"

#include <cstdio>
#include <cassert>

Mem::Mem(Bus& bus)
	: m_bus(bus)
	, m_ram()
{ }

void Mem::clock()
{
	// if (m_bus.mem_data_ready())
	// 	printf("WRITE RAM[$%04x]: $%02x\n", m_bus.read_addr(), m_bus.read_data());
	// else
	// 	printf("READ  RAM[$%04x]: $%02x\n", m_bus.read_addr(), m_ram[m_bus.read_addr()]);

	if (m_bus.mem_data_ready())
	{
		if (m_bus.read_addr() == 0xFF02 && m_bus.read_data() == 0x81)
		{
			printf("%c", m_ram[0xFF01]);
			fflush(stdout);
		}
		m_ram[m_bus.read_addr()] = m_bus.read_data();
	}
	else
		m_bus.write_data(m_ram[m_bus.read_addr()]);

	m_bus.mem_did_read_data();
}