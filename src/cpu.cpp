#include "cpu.h"
#include "cpu_instruction_name_table.h"

// temp
#include "dmg.h"

#include <cassert>
#include <cstdio>

Cpu::Cpu(Bus& bus, Mem& mem)
	: m_bus(bus)
	, m_mem(mem)
	, m_instruction_remaining_cycles(-1)
{ 
	RPC = 0x0100;
	RSP = 0xFFFE;
}

void Cpu::clock()
{
	// docs/gbctr.pdf figure 1.1

	if (m_stop)
	{
		printf("CPU STOP!\n");
		exit(0);
		return;
	}

	if (m_instruction_remaining_cycles < 0)
	{
		if (m_instruction_remaining_cycles == -1) // cpu startup: idle cpu for 1 instruction to fetch the next one
		{
			m_bus.write_addr(RPC);
			m_instruction_remaining_cycles = 0;
			return;
		}

		assert(!"m_instruction_remaining_cycles out of bounds!");
		return;
	}

	if (m_instruction_remaining_cycles == 0)
		m_instruction_byte0 = m_bus.read_data();


#if 0
	// quick and dirty debug output
	if (m_instruction_remaining_cycles == 0)
	{
		printf("$%04x:", RPC);
		if (s_instruction_names.contains(m_instruction_byte0))
		{
			int8_t instruction_size = std::get<1>(s_instruction_names.at(m_instruction_byte0));

			for (int8_t i = 0; i < instruction_size; i++)
				printf(" %02X", m_mem.direct_ram()[RPC + i]);
			for (int8_t i = instruction_size; i < 6; i++)
				printf("   ");
			
			if (m_instruction_byte0 == 0xCB) // PREFIX CB
				printf(" %s\n", s_prefixed_instruction_names[m_mem.direct_ram()[RPC + 1]]);
			else
				printf(" %s\n", std::get<0>(s_instruction_names.at(m_instruction_byte0)).c_str());
		}
		else
			printf(" %02X                %#04x\n",m_mem.direct_ram()[RPC], m_instruction_byte0);
	}
#endif


	m_instruction_remaining_cycles = execute_instruction();

	if (m_instruction_remaining_cycles == 0)
		m_bus.write_addr(RPC); // fetch next instruction
}
