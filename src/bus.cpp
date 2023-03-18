#include "bus.h"

uint8_t Bus::read_data()
{
	return m_data;
}

void Bus::write_data(uint8_t data)
{
	m_mem_data_ready = true;
	m_data = data;
}

uint16_t Bus::read_addr()
{
	return m_addr;
}

void Bus::write_addr(uint16_t addr)
{
	m_addr = addr;
}