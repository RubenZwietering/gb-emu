#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <tuple>

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
			uint8_t F0 : 1;
			uint8_t F1 : 1;
			uint8_t F2 : 1;
			uint8_t F3 : 1;
			uint8_t FC : 1;
			uint8_t FH : 1;
			uint8_t FN : 1;
			uint8_t FZ : 1;
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

	int8_t execute_instruction();
	int8_t execute_prefixed_instruction();

	static std::map<uint8_t, std::tuple<std::string, int8_t>> const s_instruction_names;
};