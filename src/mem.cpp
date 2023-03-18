#include "mem.h"

#include <cstdio>

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
		m_ram[m_bus.read_addr()] = m_bus.read_data();
	else
		m_bus.write_data(m_ram[m_bus.read_addr()]);

	m_bus.mem_did_read_data();
}