#include "cpu.h"

// temp
#include "dmg.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <map>
#include <tuple>

extern std::map<uint8_t, std::tuple<std::string, int8_t>> const s_instruction_names;

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


	// quick and dirty debug output
	if (m_instruction_remaining_cycles == 0)
	{
		printf("$%04x:", RPC);
		if (s_instruction_names.contains(m_instruction_byte0))
		{
			for (int8_t i = 0; i < std::get<1>(s_instruction_names.at(m_instruction_byte0)); i++)
				if (Dmg::s_debug_mem)
					printf(" %02X", Dmg::s_debug_mem[RPC + i]);
				else
					printf(" ??");
			for (int8_t i = std::get<1>(s_instruction_names.at(m_instruction_byte0)); i < 6; i++)
				printf("   ");
			
			printf(" %s\n", std::get<0>(s_instruction_names.at(m_instruction_byte0)).c_str());
		}
		else
		{
			if (Dmg::s_debug_mem)
				printf(" %02X                %#04x\n", Dmg::s_debug_mem[RPC], m_instruction_byte0);
			else
				printf(" ??                %#04x\n", m_instruction_byte0);
		}
	}



	m_instruction_remaining_cycles = execute_instruction();

	if (m_instruction_remaining_cycles == 0)
		m_bus.write_addr(RPC); // fetch next instruction
}

std::map<uint8_t, std::tuple<std::string, int8_t>> const s_instruction_names = std::map<uint8_t, std::tuple<std::string, int8_t>> {
	{ 0x00, { "NOP", 1 } },
	{ 0x01, { "LD BC,d16", 3 } },
	{ 0x02, { "LD (BC),A", 2 } },
	{ 0x03, { "INC BC", 1 } },
	{ 0x06, { "LD B,d8", 2 } },
	{ 0x0B, { "DEC BC", 1 } },
	{ 0x0C, { "INC C", 1 } },
	{ 0x0E, { "LD C,d8", 2 } },
	{ 0x10, { "STOP 0", 2 } },
	{ 0x11, { "LD DE,d16", 3 } },
	{ 0x13, { "INC DE", 1 } },
	{ 0x1C, { "INC E", 1 } },
	{ 0x18, { "JR r8", 2 } },
	{ 0x20, { "JR NZ,r8", 2 } },
	{ 0x21, { "LD HL,d16", 3 } },
	{ 0x23, { "INC HL", 1 } },
	{ 0x28, { "JR Z,r8", 2 } },
	{ 0x2A, { "LD A,(HL+)", 1} },
	{ 0x2C, { "INC L", 1 } },
	{ 0x30, { "JR NC,r8", 2 } },
	{ 0x31, { "LD SP,d16", 3 } },
	{ 0x33, { "INC SP", 1 } },
	{ 0x3C, { "INC A", 1 } },
	{ 0x3E, { "LD A,d8", 2 } },
	{ 0x47, { "LD B,A", 1 } },
	{ 0x4F, { "LD C,A", 1 } },
	{ 0x78, { "LD A,B", 1 } },
	{ 0x79, { "LD A,C", 1 } },
	{ 0x7C, { "LD A,H", 1 } },
	{ 0x7D, { "LD A,L", 1 } },
	{ 0xAF, { "XOR A", 1 } },
	{ 0xB1, { "OR C", 1 } },
	{ 0xB8, { "CP B", 1 } },
	{ 0xC3, { "JP a16", 3 } },
	{ 0xC1, { "POP BC", 1 } },
	{ 0xC5, { "PUSH BC", 1 } },
	{ 0xC9, { "RET", 1 } },
	{ 0xCD, { "CALL a16", 3 } },
	{ 0xD1, { "POP DE", 1 } },
	{ 0xD5, { "PUSH DE", 1 } },
	{ 0xE0, { "LDH (a8),A", 2 } },
	{ 0xE1, { "POP HL", 1 } },
	{ 0xE5, { "PUSH HL", 1 } },
	{ 0xEA, { "LD (a16),A", 3 } },
	{ 0xF0, { "LDH A,(a8)", 2 } },
	{ 0xF3, { "DI", 1 } },
	{ 0xF1, { "POP AF", 1 } },
	{ 0xF5, { "PUSH AF", 1 } },
	{ 0xFA, { "LD A,(a16)", 3 } },
	{ 0xFE, { "CP d8", 1 } },
};