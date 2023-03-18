#pragma once
#include <cstdint>

#include "bus.h"
#include "mem.h"

class Cpu
{
public:
	Cpu(Bus& bus, Mem& mem);
	~Cpu() = default;

	void clock();

private:
	Bus& m_bus;
	Mem& m_mem;

	// registers
	union {
		uint16_t RAF;
		struct {
			bool F0 : 1;
			bool F1 : 1;
			bool F2 : 1;
			bool F3 : 1;
			bool FC : 1;
			bool FH : 1;
			bool FN : 1;
			bool FZ : 1;
			uint8_t RA;
		};
	};
	union {
		uint16_t RBC;
		struct {
			uint8_t RC;
			uint8_t RB;
		};
	};
	union {
		uint16_t RDE;
		struct {
			uint8_t RE;
			uint8_t RD;
		};
	};
	union {
		uint16_t RHL;
		struct {
			uint8_t RL;
			uint8_t RH;
		};
	};
	uint16_t RSP;
	uint16_t RPC;

	bool m_stop;

	uint8_t m_instruction_byte0;
	uint8_t m_instruction_byte1;
	uint8_t m_instruction_byte2;
	int8_t m_instruction_remaining_cycles;
	bool m_instruction_prefix_mode;

	int8_t execute_instruction();
};