#include "cpu.h"

#include <cstdio>
#include <cassert>

constexpr uint8_t lsb(uint16_t u16)
{
	return static_cast<uint8_t>(u16 & 0x00FF);
}
constexpr uint8_t msb(uint16_t u16)
{
	return static_cast<uint8_t>((u16 & 0xFF00) >> 8);
}
constexpr void set_lsb(uint16_t& u16, uint8_t u8)
{
	u16 = (u16 & 0xFF00) | static_cast<uint16_t>(u8);
}
constexpr void set_msb(uint16_t& u16, uint8_t u8)
{
	u16 = (static_cast<uint16_t>(u8) << 8) | (u16 & 0x00FF);
}

// https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html

int8_t Cpu::execute_instruction()
{
	static auto const OR_r = [&](uint8_t& r) -> int8_t {
		++RPC;
		RA |= r;
		FZ = (RA == 0);
		FN = false;
		FH = false;
		FC = false;
		return 0;
	};
	static auto const INC_r = [&](uint8_t& r) -> int8_t {
		++RPC;
		++r;
		FZ = (r == 0);
		FN = false;
		FH = (r == 0x10);
		return 0;
	};
	static auto const INC_rr = [&](uint16_t& rr) -> int8_t {
		if (m_instruction_remaining_cycles == 0)
		{
			++RPC;
			++rr;
			return 1;
		}

		if (m_instruction_remaining_cycles == 1)
		{ 
			// The gameboy most likely increments a byte per machine cycle,
			// so there is nothing to do here 
			return 0;
		}

		return -2;
	};
	static auto const DEC_rr = [&](uint16_t& rr) -> int8_t {
		if (m_instruction_remaining_cycles == 0)
		{
			++RPC;
			--rr;
			return 1;
		}

		if (m_instruction_remaining_cycles == 1)
		{ 
			// The gameboy most likely decrements a byte per machine cycle,
			// so there is nothing to do here 
			return 0;
		}

		return -2;
	};
	static auto const LD_rr_nn = [&](uint16_t& rr) -> int8_t {
		if (m_instruction_remaining_cycles == 0)
		{
			++RPC;
			m_bus.write_addr(RPC); 
			return 2;
		}

		if (m_instruction_remaining_cycles == 2)
		{
			++RPC;
			set_lsb(rr, m_bus.read_data());
			m_bus.write_addr(RPC); 
			return 1;
		}

		if (m_instruction_remaining_cycles == 1)
		{
			set_msb(rr, m_bus.read_data());
			++RPC;
			return 0;
		}

		return -2;
	};
	static auto const POP_rr = [&](uint16_t& rr) -> int8_t {
		if (m_instruction_remaining_cycles == 0)
		{
			++RPC;
			m_bus.write_addr(RSP);
			return 2;
		}
		
		if (m_instruction_remaining_cycles == 2)
		{
			set_lsb(rr, m_bus.read_data());
			++RSP;
			m_bus.write_addr(RSP);
			return 1;
		}

		if (m_instruction_remaining_cycles == 1)
		{
			set_msb(rr, m_bus.read_data());
			++RSP;
			return 0;
		}

		return -2;
	};
	static auto const PUSH_rr = [&](uint16_t& rr) -> int8_t {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				return 3;
			}
			
			if (m_instruction_remaining_cycles == 3)
			{
				--RSP;
				m_bus.write_addr(RSP);
				m_bus.write_data(msb(rr)); // msb
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				--RSP;
				m_bus.write_addr(RSP);
				m_bus.write_data(lsb(rr)); // lsb
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return 0;
			}
		return -2;
	};


	if (m_instruction_prefix_mode)
	{
		switch (m_instruction_byte0)
		{
			default:
				printf("uninplemented instruction with prefix %#x\n", m_instruction_byte0);
				return -2;
		}
	}

	switch (m_instruction_byte0)
	{
		//   NOP
		//  1   4
		// - - - -
		case 0x00: {
			++RPC;
			return 0;
		};
		// STOP 0
		//  2   4
		// - - - -
		case 0x10: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				++RPC;
				m_stop = true;
				return 0;
			}

			return -2;
		};
		// JR NZ,r8
		// 2  12/8
		// - - - -
		case 0x20: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return FZ ? 1 : 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				++RPC;
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				if (FZ)
				{
					++RPC;
					(void)m_bus.read_data();
				}
				else
				{
					RPC += static_cast<int8_t>(m_bus.read_data());
				}
			}
		} break;
		// JR NC,r8
		// 2  12/8
		// - - - -
		case 0x30: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return FC ? 1 : 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				++RPC;
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				if (FC)
				{
					++RPC;
					(void)m_bus.read_data();
				}
				else
				{
					RPC += static_cast<int8_t>(m_bus.read_data());	
				}
			}
		} break;

		//  JR r8
		//  2  12
		// - - - -
		case 0x18: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				++RPC;
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				RPC += static_cast<int8_t>(m_bus.read_data());	
			}
		} break;

		
		// JR Z,r8
		// 2  12/8
		// - - - -
		case 0x28: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return FZ ? 2 : 1;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				++RPC;
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				if (FZ)
				{
					RPC += static_cast<int8_t>(m_bus.read_data());
				}
				else
				{
					++RPC;
					(void)m_bus.read_data();
				}
			}
		} break;

		// LD BC,d16
		// 	3  12
		// - - - -
		case 0x01: return LD_rr_nn(RBC);
		// LD DE,d16
		// 	3  12
		// - - - -
		case 0x11: return LD_rr_nn(RDE);
		// LD HL,d16
		// 	3  12
		// - - - -
		case 0x21: return LD_rr_nn(RHL);
		// LD SP,d16
		// 	3  12
		// - - - -
		case 0x31: return LD_rr_nn(RSP);

		// LD (BC),A
		//  1   8
		// - - - -
		case 0x02: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RBC); 
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
				m_bus.write_addr(RA); 
		} break;

		// INC BC
		//  1   8
		// - - - -
		case 0x03: return INC_rr(RBC);
		// INC DE
		//  1   8
		// - - - -
		case 0x13: return INC_rr(RDE);
		// INC HL
		//  1   8
		// - - - -
		case 0x23: return INC_rr(RHL);
		// INC SP
		//  1   8
		// - - - -
		case 0x33: return INC_rr(RSP);

		// DEC BC
		//  1   8
		// - - - -
		case 0x0B: return DEC_rr(RBC);

		//  INC C
		//  1   4
		// Z 0 H -
		case 0x0C: return INC_r(RC);
		//  INC E
		//  1   4
		// Z 0 H -
		case 0x1C: return INC_r(RE);
		//  INC L
		//  1   4
		// Z 0 H -
		case 0x2C: return INC_r(RL);
		//  INC A
		//  1   4
		// Z 0 H -
		case 0x3C: return INC_r(RA);

		// LD B,d8
		//  2   8
		// - - - -
		case 0x06: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				++RPC;
				RB = m_bus.read_data();
			}
		} break;
		
		// LD C,d8
		//  2   8
		// - - - -
		case 0x0E: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				++RPC;
				RE = m_bus.read_data();
			}
		} break;
		// LD A,d8
		//  2   8
		// - - - -
		case 0x3E: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				++RPC;
				RA = m_bus.read_data();
			}
		} break;

		// LD B,A
		//  1   4
		// - - - -
		case 0x47: {
			++RPC;
			RB = RA;
		} break;

		// LD C,A
		//  1   4
		// - - - -
		case 0x4F: {
			++RPC;
			RC = RA;
		} break;
		
		// LD A,B
		//  1   4
		// - - - -
		case 0x78: {
			++RPC;
			RA = RB;
		} break;
		// LD A,C
		//  1   4
		// - - - -
		case 0x79: {
			++RPC;
			RA = RC;
		} break;

		// LD A,H
		//  1   4
		// - - - -
		case 0x7C: {
			++RPC;
			RA = RL;
		} break;
		
		// LD A,L
		//  1   4
		// - - - -
		case 0x7D: {
			++RPC;
			RA = RL;
		} break;

		//  XOR A
		//  1   4
		// Z 0 0 0
		case 0xAF: {
			++RPC;
			RA ^= RA; 
			FZ = (RA == 0); 
			FN = false;
			FH = false;
			FC = false;
		} break;

		//  OR C
		//  1   4
		// Z 0 0 0
		case 0xB1: return OR_r(RC);

		//  CP B
		//  1   4
		// Z 1 H C
		case 0xB8: {
			++RPC;
			uint8_t RT = RA - RB; // TODO: check if A is thrashed or not
			FZ = (RT == 0);
			FN = true;
			FH = (RB & 0xf) > (RA & 0xf); // TODO: check if this is correct
			FC = RB > RA;
			return 0;
		};
		//  CP d8
		//  2   8
		// Z 1 H C
		case 0xFE: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				++RPC;
				uint8_t RT = m_bus.read_data();
				
				RT = RA - RT; // TODO: check if A is thrashed or not
				FZ = (RT == 0);
				FN = true;
				FH = (RT & 0xf) > (RA & 0xf); // TODO: check if this is correct
				FC = RT > RA;
				return 0;
			}

			return -2;
		};
		
		// POP BC
		//  1  12
		// - - - -
		case 0xC1: return POP_rr(RBC);
		// POP DE
		//  1  12
		// - - - -
		case 0xD1: return POP_rr(RDE);
		// POP HL
		//  1  12
		// - - - -
		case 0xE1: return POP_rr(RHL);
		// POP AF
		//  1  12
		// - - - -
		case 0xF1: return POP_rr(RAF);
		
		// PUSH BC
		//  1  16
		// - - - -
		case 0xC5: return PUSH_rr(RBC);
		// PUSH DE
		//  1  16
		// - - - -
		case 0xD5: return PUSH_rr(RDE);
		// PUSH HL
		//  1  16
		// - - - -
		case 0xE5: return PUSH_rr(RHL);
		// PUSH AF
		//  1  16
		// - - - -
		case 0xF5: return PUSH_rr(RAF);

		// JP a16
		//  3  16
		// - - - -
		case 0xC3: { // TODO: check if this instruction has the correct execution length
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 3;
			}

			if (m_instruction_remaining_cycles == 3)
			{
				++RPC;
				m_instruction_byte1 = m_bus.read_data(); // lsb
				m_bus.write_addr(RPC); 
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				++RPC;
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				RPC = static_cast<uint16_t>(m_bus.read_data()) << 8 | m_instruction_byte1;
			}
		} break;

		//   RET
		//  1  16
		// - - - -
		case 0xC9: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RSP);
				++RSP;
				return 3;
			}

			if (m_instruction_remaining_cycles == 3)
			{
				m_instruction_byte1 = m_bus.read_data(); // lsb
				m_bus.write_addr(RSP);
				++RSP;
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				RPC = static_cast<uint16_t>(m_bus.read_data()) << 8 | m_instruction_byte1; // msb
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{

			}
		} break;

		// CALL a16
		//  3  24
		// - - - -
		case 0xCD: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 5;
			}

			if (m_instruction_remaining_cycles == 5)
			{
				++RPC;
				m_instruction_byte1 = m_bus.read_data(); // lsb
				m_bus.write_addr(RPC); 
				return 4;
			}

			if (m_instruction_remaining_cycles == 4)
			{
				++RPC;
				m_instruction_byte2 = static_cast<uint8_t>(RPC & 0x00FF); // lsb
				RPC = (RPC & 0xFF00) | m_instruction_byte1; // lsb
				m_instruction_byte1 = static_cast<uint8_t>((RPC & 0xFF00) >> 8); // msb
				RPC = (RPC & 0x00FF) | static_cast<uint16_t>(m_bus.read_data()) << 8; // msb
				return 3;
			}

			if (m_instruction_remaining_cycles == 3)
			{
				--RSP;
				m_bus.write_addr(RSP);
				m_bus.write_data(m_instruction_byte1); // msb
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				--RSP;
				m_bus.write_addr(RSP);
				m_bus.write_data(m_instruction_byte2); // lsb
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{

			}
		} break;

		// LD A,(HL+)
		//  1   8
		// - - - -
		case 0x2A: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RHL); 
				++RHL;
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				RA = m_bus.read_data();
			}
		} break;

		// LDH (a8),A
		//  2  12
		// - - - -
		case 0xE0: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				++RPC;
				m_bus.write_addr(0xFF00 | m_bus.read_data()); // lsb

				m_bus.write_data(RA);
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return 0;
			}

			return -2;
		};
		// LDH A,(a8)
		//  2  12
		// - - - -
		case 0xF0: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				++RPC;
				m_bus.write_addr(0xFF00 | m_bus.read_data()); // lsb
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				RA = m_bus.read_data();
				return 0;
			}

			return -2;
		};

		// LD (a16),A
		//  3  16
		// - - - -
		case 0xEA: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 3;
			}

			if (m_instruction_remaining_cycles == 3)
			{
				++RPC;
				m_instruction_byte1 = m_bus.read_data(); // lsb
				m_bus.write_addr(RPC); 
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				++RPC;
				m_bus.write_addr(static_cast<uint16_t>(m_bus.read_data()) << 8 | m_instruction_byte1); // msb

				m_bus.write_data(RA);
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return 0;
			}

			return -2;
		};
		// LD A,(a16)
		//  3  16
		// - - - -
		case 0xFA: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 3;
			}

			if (m_instruction_remaining_cycles == 3)
			{
				++RPC;
				m_instruction_byte1 = m_bus.read_data(); // lsb
				m_bus.write_addr(RPC); 
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				++RPC;
				m_bus.write_addr(static_cast<uint16_t>(m_bus.read_data()) << 8 | m_instruction_byte1); // msb
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				RA = m_bus.read_data();
				return 0;
			}
			return -2;
		};

		//   DI
		//  1   4
		// - - - -
		case 0xF3: {
			++RPC;
			printf("Interrupts not yet implemented.\n"); // TODO
		} break;

		default:
			printf("uninplemented instruction %#4x\n", m_instruction_byte0);

			// halt cpu
			return -2;
	}


	return 0;
}