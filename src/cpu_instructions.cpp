#include "cpu.h"
#include "cpu_instruction_name_table.h"

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
	static auto const ADD_r = [&](uint8_t r) -> int8_t {
		++RPC;
		FN = 0;
		// FH = ??;
		// FC = ??;

		RA += r;
		FZ = (RA == 0);
		return 0;
	};
	static auto const ADD_HL_rr = [&](uint16_t& rr) -> int8_t {
		if (m_instruction_remaining_cycles == 0)
		{
			++RPC;
			FN = 0;
			// FH = ??;
			// FC = ??;

			RHL += rr;
			return 1;
		}

		if (m_instruction_remaining_cycles == 1)
		{
			return 0;
		}

		return -2;
	};
	static auto const ADC_r = [&](uint8_t r) -> int8_t {
		return ADD_r(r + FC); // TODO
		return 0;
	};
	static auto const SUB_r = [&](uint8_t r) -> int8_t {
		++RPC;
		FN = 1;
		FH = (RA & 0xf) < (r & 0xf);
		FC = RA < r;
		
		RA -= r;
		FZ = (RA == 0);
		return 0;
	};
	static auto const SBC_r = [&](uint8_t r) -> int8_t {
		return SUB_r(r + FC); // TODO
		return 0;
	};
	static auto const AND_r = [&](uint8_t r) -> int8_t {
		++RPC;
		RA &= r;
		FZ = (RA == 0);
		FN = 0;
		FH = 1;
		FC = 0;
		return 0;
	};
	static auto const XOR_r = [&](uint8_t r) -> int8_t {
		++RPC;
		RA ^= r;
		FZ = (RA == 0);
		FN = 0;
		FH = 0;
		FC = 0;
		return 0;
	};
	static auto const OR_r = [&](uint8_t r) -> int8_t {
		++RPC;
		RA |= r;
		FZ = (RA == 0);
		FN = 0;
		FH = 0;
		FC = 0;
		return 0;
	};
	static auto const CP_r = [&](uint8_t r) -> int8_t {
		++RPC;
		uint8_t RT = RA - RB; // TODO: check if A is thrashed or not
		FZ = (RT == 0);
		FN = 1;
		FH = (RB & 0xf) > (RA & 0xf); // TODO: check if this is correct
		FC = RB > RA;
		return 0;
	};
	static auto const INC_r = [&](uint8_t& r) -> int8_t {
		++RPC;
		++r;
		FZ = (r == 0);
		FN = 0;
		FH = (r & 0x0F) == 0x00;
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
	static auto const DEC_r = [&](uint8_t& r) -> int8_t {
		++RPC;
		--r;
		FZ = (r == 0);
		FN = 1;
		FH = (r & 0x0F) == 0x0F;
		return 0;
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
	static auto const LD_r_r = [&](uint8_t& r1, uint8_t& r2) -> int8_t {
		++RPC;
		r1 = r2;
		return 0;
	};
	static auto const LD_r_n = [&](uint8_t& r) -> int8_t {
		if (m_instruction_remaining_cycles == 0)
		{
			++RPC;
			m_bus.write_addr(RPC); 
			return 1;
		}
		
		if (m_instruction_remaining_cycles == 1)
		{
			++RPC;
			r = m_bus.read_data();
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
	static auto const LD_rr_address_r = [&](uint16_t& rr, uint8_t& r) -> int8_t  {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(rr);
				m_bus.write_data(r);
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return 0;
			}

			return -2;
	};
	static auto const LD_r_rr_address = [&](uint8_t& r, uint16_t& rr) -> int8_t  {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(rr); 
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				r = m_bus.read_data();
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
	static auto const RET_cc = [&](uint8_t cc) -> int8_t {
		// TODO: change behaviour to branch in the middle of the instruction maybe?
		if (cc)
		{
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RSP);
				++RSP;
				return 4;
			}

			if (m_instruction_remaining_cycles == 4)
			{
				m_instruction_byte1 = m_bus.read_data(); // lsb
				m_bus.write_addr(RSP);
				++RSP;
				return 3;
			}

			if (m_instruction_remaining_cycles == 3)
			{
				RPC = static_cast<uint16_t>(m_bus.read_data()) << 8 | m_instruction_byte1; // msb
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return 0;
			}
		}
		else
		{
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return 0;
			}
		}
		return -2;
	};
	static auto const JR_cc_n = [&](uint8_t cc) -> int8_t {
		if (cc)
		{
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				++RPC;
				m_instruction_byte1 = m_bus.read_data();
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				RPC += static_cast<int8_t>(m_instruction_byte1);
				return 0;
			}
		}
		else
		{
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				++RPC;
				(void)m_bus.read_data();
				return 0;
			}
		}

		return -2;
	};
	static auto const JP_cc_nn = [&](uint8_t cc) -> int8_t {
			if (cc)
			{
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
					m_instruction_byte2 = m_bus.read_data(); // msb
					return 1;
				}

				if (m_instruction_remaining_cycles == 1)
				{
					RPC = static_cast<uint16_t>(m_instruction_byte2) << 8 | m_instruction_byte1;
					return 0;
				}
			}
			else
			{
				if (m_instruction_remaining_cycles == 0)
				{
					++RPC;
					m_bus.write_addr(RPC); 
					return 2;
				}

				if (m_instruction_remaining_cycles == 2)
				{
					++RPC;
					(void)m_bus.read_data();
					m_bus.write_addr(RPC); 
					return 1;
				}

				if (m_instruction_remaining_cycles == 1)
				{
					++RPC;
					(void)m_bus.read_data();
					return 0;
				}
			}

			return -2;
	};
	static auto const RST_nn = [&](uint8_t nn) -> int8_t {
		if (m_instruction_remaining_cycles == 0)
		{
			++RPC;
			m_instruction_byte1 = msb(RPC);
			m_instruction_byte2 = lsb(RPC);
			RPC = 0xFF00 | nn;
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
			return 0;
		}
		
		return -2;
	};

	switch (m_instruction_byte0)
	{
		// ADD HL,BC
		//  1   8
		// - 0 H C
		case 0x09: ADD_HL_rr(RBC);
		// ADD HL,DE
		//  1   8
		// - 0 H C
		case 0x19: ADD_HL_rr(RDE);
		// ADD HL,HL
		//  1   8
		// - 0 H C
		case 0x29: ADD_HL_rr(RHL);
		// ADD HL,SP
		//  1   8
		// - 0 H C
		case 0x39: ADD_HL_rr(RSP);

		
		//  RLCA
		//  1   4
		// 0 0 0 C
		case 0x07: {
			++RPC;
			FZ = 0;
			FN = 0;
			FH = 0;
			FC = RA & 0b10000000;

			RA = ((RA << 1) & 0x11111110) | ((RA >> 7) & 0b00000001);

			return 0;
		};
		//  RRCA
		//  1   4
		// 0 0 0 C
		case 0x0F: {
			++RPC;
			FZ = 0;
			FN = 0;
			FH = 0;
			FC = RA & 0b00000001;

			RA = ((RA >> 1) & 0x01111111) | ((RA << 7) & 0b10000000);

			return 0;
		};
		//   RLA
		//  1   4
		// 0 0 0 C
		case 0x17: {
			++RPC;
			FZ = 0;
			FN = 0;
			FH = 0;
			uint8_t FT = FC;
			FC = (RA & 0b10000000);

			RA <<= 1;
			RA &= 0b11111110;
			RA |= FT & 0b00000001;
			return 0;
		};
		//   RRA
		//  1   4
		// 0 0 0 C
		case 0x1F: {
			++RPC;
			FZ = 0;
			FN = 0;
			FH = 0;
			uint8_t FT = FC;
			FC = (RA & 0b00000001);

			RA >>= 1;
			RA &= 0b01111111;
			RA |= (FT << 7) & 0b10000000;
			return 0;
		};
		//   CPL
		//  1   4
		// - 1 1 -
		case 0x2F: {
			++RPC;
			FN = 1;
			FH = 1;

			RA ^= 0xFF;
			return 0;
		};
		//   SCF
		//  1   4
		// - 0 0 1
		case 0x37: {
			++RPC;
			FN = 0;
			FH = 0;
			FC = 1;
			return 0;
		};
		//   CCF
		//  1   4
		// - 0 0 C
		case 0x3F: {
			++RPC;
			FN = 0;
			FH = 0;
			FC ^= 1;
			return 0;
		};

		//  JR r8
		//  2  12
		// - - - -
		case 0x18: return JR_cc_n(1);
		// JR NZ,r8
		// 2  12/8
		// - - - -
		case 0x20: return JR_cc_n(!FN);
		// JR Z,r8
		// 2  12/8
		// - - - -
		case 0x28: return JR_cc_n(FZ);
		// JR NC,r8
		// 2  12/8
		// - - - -
		case 0x30: return JR_cc_n(!FC);
		// JR C,r8
		// 2  12/8
		// - - - -
		case 0x38: return JR_cc_n(FC);
		

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
		case 0x02: return LD_rr_address_r(RBC, RA);
		// LD (DE),A
		//  1   8
		// - - - -
		case 0x12: return LD_rr_address_r(RDE, RA);
		// LD (HL+),A
		//  1   8
		// - - - -
		case 0x22: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RHL); 
				++RHL;
				m_bus.write_data(RA);
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				return 0;
			}
			return -2;
		};
		// LD (HL-),A
		//  1   8
		// - - - -
		case 0x32: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RHL);
				m_bus.write_data(RA);
				--RHL;
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				return 0;
			}
			return -2;
		};


		// LD (HL),B
		//  1   8
		// - - - -
		case 0x70: return LD_rr_address_r(RHL, RB);
		// LD (HL),C
		//  1   8
		// - - - -
		case 0x71: return LD_rr_address_r(RHL, RC);
		// LD (HL),D
		//  1   8
		// - - - -
		case 0x72: return LD_rr_address_r(RHL, RD);
		// LD (HL),E
		//  1   8
		// - - - -
		case 0x73: return LD_rr_address_r(RHL, RE);
		// LD (HL),H
		//  1   8
		// - - - -
		case 0x74: return LD_rr_address_r(RHL, RH);
		// LD (HL),L
		//  1   8
		// - - - -
		case 0x75: return LD_rr_address_r(RHL, RL);
		// LD (HL),A
		//  1   8
		// - - - -
		case 0x77: return LD_rr_address_r(RHL, RA);


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
		// DEC DE
		//  1   8
		// - - - -
		case 0x1B: return DEC_rr(RDE);
		// DEC HL
		//  1   8
		// - - - -
		case 0x2B: return DEC_rr(RHL);
		// DEC SP
		//  1   8
		// - - - -
		case 0x3B: return DEC_rr(RSP);

		//  INC B
		//  1   4
		// Z 0 H -
		case 0x04: return INC_r(RB);
		//  INC D
		//  1   4
		// Z 0 H -
		case 0x14: return INC_r(RD);
		//  INC H
		//  1   4
		// Z 0 H -
		case 0x24: return INC_r(RH);
		//  INC (HL)
		//  1   12
		// Z 0 H -
		case 0x34: {
			if (m_instruction_remaining_cycles == 0)
			{
				m_bus.write_addr(RHL);

				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				m_bus.write_addr(RHL);

				uint8_t RT = m_bus.read_data();
				(void)INC_r(RT);
				m_bus.write_data(RT);

				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return 0;
			}
			return -2;
		};

		//  DEC B
		//  1   4
		// Z 1 H -
		case 0x05: return DEC_r(RB);
		//  DEC D
		//  1   4
		// Z 1 H -
		case 0x15: return DEC_r(RD);
		//  DEC H
		//  1   4
		// Z 1 H -
		case 0x25: return DEC_r(RH);
		//  DEC (HL)
		//  1   12
		// Z 1 H -
		case 0x35: {
			if (m_instruction_remaining_cycles == 0)
			{
				m_bus.write_addr(RHL);

				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				m_bus.write_addr(RHL);

				uint8_t RT = m_bus.read_data();
				(void)DEC_r(RT);
				m_bus.write_data(RT);

				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return 0;
			}
			return -2;
		};


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
		
		//  DEC C
		//  1   4
		// Z 1 H -
		case 0x0D: return DEC_r(RC);
		//  DEC E
		//  1   4
		// Z 1 H -
		case 0x1D: return DEC_r(RE);
		//  DEC L
		//  1   4
		// Z 1 H -
		case 0x2D: return DEC_r(RL);
		//  DEC A
		//  1   4
		// Z 1 H -
		case 0x3D: return DEC_r(RA);

		// LD B,d8
		//  2   8
		// - - - -
		case 0x06: return LD_r_n(RB);
		// LD D,d8
		//  2   8
		// - - - -
		case 0x16: return LD_r_n(RD);
		// LD H,d8
		//  2   8
		// - - - -
		case 0x26: return LD_r_n(RH);
		// LD (HL),d8
		//  2   12
		// - - - -
		case 0x36: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC);
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				++RPC;
				m_bus.write_addr(RHL);
				m_bus.write_data(m_bus.read_data());
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return 0;
			}

			return -1;
		};

		// LD C,d8
		//  2   8
		// - - - -
		case 0x0E: return LD_r_n(RC);
		// LD E,d8
		//  2   8
		// - - - -
		case 0x1E: return LD_r_n(RE);
		// LD L,d8
		//  2   8
		// - - - -
		case 0x2E: return LD_r_n(RL);
		// LD A,d8
		//  2   8
		// - - - -
		case 0x3E: return LD_r_n(RA);
		
		// LD B,B
		//  1   4
		// - - - -
		case 0x40: return LD_r_r(RB, RB);
		// LD B,C
		//  1   4
		// - - - -
		case 0x41: return LD_r_r(RB, RC);
		// LD B,D
		//  1   4
		// - - - -
		case 0x42: return LD_r_r(RB, RD);
		// LD B,E
		//  1   4
		// - - - -
		case 0x43: return LD_r_r(RB, RE);
		// LD B,H
		//  1   4
		// - - - -
		case 0x44: return LD_r_r(RB, RH);
		// LD B,L
		//  1   4
		// - - - -
		case 0x45: return LD_r_r(RB, RL);
		// LD B,(HL)
		//  1   8
		// - - - -
		case 0x46: return LD_r_rr_address(RB, RHL);
		// LD B,A
		//  1   4
		// - - - -
		case 0x47: return LD_r_r(RB, RA);

		// LD C,B
		//  1   4
		// - - - -
		case 0x48: return LD_r_r(RC, RB);
		// LD C,C
		//  1   4
		// - - - -
		case 0x49: return LD_r_r(RC, RC);
		// LD C,D
		//  1   4
		// - - - -
		case 0x4A: return LD_r_r(RC, RD);
		// LD C,E
		//  1   4
		// - - - -
		case 0x4B: return LD_r_r(RC, RE);
		// LD C,H
		//  1   4
		// - - - -
		case 0x4C: return LD_r_r(RC, RH);
		// LD C,L
		//  1   4
		// - - - -
		case 0x4D: return LD_r_r(RC, RL);
		// LD C,(HL)
		//  1   8
		// - - - -
		case 0x4E: return LD_r_rr_address(RC, RHL);
		// LD C,A
		//  1   4
		// - - - -
		case 0x4F: return LD_r_r(RC, RA);
		
		// LD D,B
		//  1   4
		// - - - -
		case 0x50: return LD_r_r(RD, RB);
		// LD D,C
		//  1   4
		// - - - -
		case 0x51: return LD_r_r(RD, RC);
		// LD D,D
		//  1   4
		// - - - -
		case 0x52: return LD_r_r(RD, RD);
		// LD D,E
		//  1   4
		// - - - -
		case 0x53: return LD_r_r(RD, RE);
		// LD D,H
		//  1   4
		// - - - -
		case 0x54: return LD_r_r(RD, RH);
		// LD D,L
		//  1   4
		// - - - -
		case 0x55: return LD_r_r(RD, RL);
		// LD D,(HL)
		//  1   8
		// - - - -
		case 0x56: return LD_r_rr_address(RD, RHL);
		// LD D,A
		//  1   4
		// - - - -
		case 0x57: return LD_r_r(RD, RA);
		
		// LD E,B
		//  1   4
		// - - - -
		case 0x58: return LD_r_r(RE, RB);
		// LD E,C
		//  1   4
		// - - - -
		case 0x59: return LD_r_r(RE, RC);
		// LD E,D
		//  1   4
		// - - - -
		case 0x5A: return LD_r_r(RE, RD);
		// LD E,E
		//  1   4
		// - - - -
		case 0x5B: return LD_r_r(RE, RE);
		// LD E,H
		//  1   4
		// - - - -
		case 0x5C: return LD_r_r(RE, RH);
		// LD E,L
		//  1   4
		// - - - -
		case 0x5D: return LD_r_r(RE, RL);
		// LD E,(HL)
		//  1   8
		// - - - -
		case 0x5E: return LD_r_rr_address(RE, RHL);
		// LD E,A
		//  1   4
		// - - - -
		case 0x5F: return LD_r_r(RE, RA);
		
		// LD H,B
		//  1   4
		// - - - -
		case 0x60: return LD_r_r(RH, RB);
		// LD H,C
		//  1   4
		// - - - -
		case 0x61: return LD_r_r(RH, RC);
		// LD H,D
		//  1   4
		// - - - -
		case 0x62: return LD_r_r(RH, RD);
		// LD H,E
		//  1   4
		// - - - -
		case 0x63: return LD_r_r(RH, RE);
		// LD H,H
		//  1   4
		// - - - -
		case 0x64: return LD_r_r(RH, RH);
		// LD H,L
		//  1   4
		// - - - -
		case 0x65: return LD_r_r(RH, RL);
		// LD H,(HL)
		//  1   8
		// - - - -
		case 0x66: return LD_r_rr_address(RH, RHL);
		// LD H,A
		//  1   4
		// - - - -
		case 0x67: return LD_r_r(RH, RA);

		// LD L,B
		//  1   4
		// - - - -
		case 0x68: return LD_r_r(RL, RB);
		// LD L,C
		//  1   4
		// - - - -
		case 0x69: return LD_r_r(RL, RC);
		// LD L,D
		//  1   4
		// - - - -
		case 0x6A: return LD_r_r(RL, RD);
		// LD L,E
		//  1   4
		// - - - -
		case 0x6B: return LD_r_r(RL, RE);
		// LD L,H
		//  1   4
		// - - - -
		case 0x6C: return LD_r_r(RL, RH);
		// LD L,L
		//  1   4
		// - - - -
		case 0x6D: return LD_r_r(RL, RL);
		// LD L,(HL)
		//  1   8
		// - - - -
		case 0x6E: return LD_r_rr_address(RL, RHL);
		// LD L,A
		//  1   4
		// - - - -
		case 0x6F: return LD_r_r(RL, RA);

		// LD A,B
		//  1   4
		// - - - -
		case 0x78: return LD_r_r(RA, RB);
		// LD A,C
		//  1   4
		// - - - -
		case 0x79: return LD_r_r(RA, RC);
		// LD A,D
		//  1   4
		// - - - -
		case 0x7A: return LD_r_r(RA, RD);
		// LD A,E
		//  1   4
		// - - - -
		case 0x7B: return LD_r_r(RA, RE);
		// LD A,H
		//  1   4
		// - - - -
		case 0x7C: return LD_r_r(RA, RH);
		// LD A,L
		//  1   4
		// - - - -
		case 0x7D: return LD_r_r(RA, RL);
		// LD A,(HL)
		//  1   8
		// - - - -
		case 0x7E: return LD_r_rr_address(RA, RHL);
		// LD A,A
		//  1   4
		// - - - -
		case 0x7F: return LD_r_r(RA, RA);

		
		// ADD A,B
		//  1   4
		// Z 0 H C
		case 0x80: return ADD_r(RB);
		// ADD A,C
		//  1   4
		// Z 0 H C
		case 0x81: return ADD_r(RC);
		// ADD A,D
		//  1   4
		// Z 0 H C
		case 0x82: return ADD_r(RD);
		// ADD A,E
		//  1   4
		// Z 0 H C
		case 0x83: return ADD_r(RE);
		// ADD A,H
		//  1   4
		// Z 0 H C
		case 0x84: return ADD_r(RH);
		// ADD A,L
		//  1   4
		// Z 0 H C
		case 0x85: return ADD_r(RL);
		// ADD A,(HL)
		//  1   8
		// Z 0 H C
		case 0x86: {
			if (m_instruction_remaining_cycles == 0)
			{
				m_bus.write_addr(RHL);

				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return ADD_r(m_bus.read_data());
			}

			return -2;
		};
		// ADD A,A
		//  1   4
		// Z 0 H C
		case 0x87: return ADD_r(RA);
		
		// ADC A,B
		//  1   4
		// Z 0 H C
		case 0x88: return ADC_r(RB);
		// ADC A,C
		//  1   4
		// Z 0 H C
		case 0x89: return ADC_r(RC);
		// ADC A,D
		//  1   4
		// Z 0 H C
		case 0x8A: return ADC_r(RD);
		// ADC A,E
		//  1   4
		// Z 0 H C
		case 0x8B: return ADC_r(RE);
		// ADC A,H
		//  1   4
		// Z 0 H C
		case 0x8C: return ADC_r(RH);
		// ADC A,L
		//  1   4
		// Z 0 H C
		case 0x8D: return ADC_r(RL);
		// ADC A,(HL)
		//  1   8
		// Z 0 H C
		case 0x8E: {
			if (m_instruction_remaining_cycles == 0)
			{
				m_bus.write_addr(RHL);

				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return ADC_r(m_bus.read_data());
			}

			return -2;
		};
		// ADC A,A
		//  1   4
		// Z 0 H C
		case 0x8F: return ADC_r(RA);

		//  SUB B
		//  1   4
		// Z 1 H C
		case 0x90: return SUB_r(RB);
		//  SUB C
		//  1   4
		// Z 1 H C
		case 0x91: return SUB_r(RC);
		//  SUB D
		//  1   4
		// Z 1 H C
		case 0x92: return SUB_r(RD);
		//  SUB E
		//  1   4
		// Z 1 H C
		case 0x93: return SUB_r(RE);
		//  SUB H
		//  1   4
		// Z 1 H C
		case 0x94: return SUB_r(RH);
		//  SUB L
		//  1   4
		// Z 1 H C
		case 0x95: return SUB_r(RL);
		// SUB (HL)
		//  1   8
		// Z 1 H C
		case 0x96: {
			if (m_instruction_remaining_cycles == 0)
			{
				m_bus.write_addr(RHL);

				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return SUB_r(m_bus.read_data());
			}

			return -2;
		};
		//  SUB A
		//  1   4
		// Z 1 H C
		case 0x97: return SUB_r(RA);
		
		// SBC A,B
		//  1   4
		// Z 1 H C
		case 0x98: return SBC_r(RB);
		// SBC A,C
		//  1   4
		// Z 1 H C
		case 0x99: return SBC_r(RC);
		// SBC A,D
		//  1   4
		// Z 1 H C
		case 0x9A: return SBC_r(RD);
		// SBC A,E
		//  1   4
		// Z 1 H C
		case 0x9B: return SBC_r(RE);
		// SBC A,H
		//  1   4
		// Z 1 H C
		case 0x9C: return SBC_r(RH);
		// SBC A,L
		//  1   4
		// Z 1 H C
		case 0x9D: return SBC_r(RL);
		// SBC A,(HL)
		//  1   8
		// Z 1 H C
		case 0x9E: {
			if (m_instruction_remaining_cycles == 0)
			{
				m_bus.write_addr(RHL);

				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return SBC_r(m_bus.read_data());
			}

			return -2;
		};
		// SBC A,A
		//  1   4
		// Z 1 H C
		case 0x9F: return SBC_r(RA);

		//  AND B
		//  1   4
		// Z 0 1 0
		case 0xA0: return AND_r(RB);
		//  AND C
		//  1   4
		// Z 0 1 0
		case 0xA1: return AND_r(RC);
		//  AND D
		//  1   4
		// Z 0 1 0
		case 0xA2: return AND_r(RD);
		//  AND E
		//  1   4
		// Z 0 1 0
		case 0xA3: return AND_r(RE);
		//  AND H
		//  1   4
		// Z 0 1 0
		case 0xA4: return AND_r(RH);
		//  AND L
		//  1   4
		// Z 0 1 0
		case 0xA5: return AND_r(RL);
		// AND (HL)
		//  1   8
		// Z 0 1 0
		case 0xA6: {
			if (m_instruction_remaining_cycles == 0)
			{
				m_bus.write_addr(RHL);

				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return AND_r(m_bus.read_data());
			}

			return -2;
		};
		//  AND A
		//  1   4
		// Z 0 1 0
		case 0xA7: return AND_r(RA);

		//  XOR B
		//  1   4
		// Z 0 0 0
		case 0xA8: return XOR_r(RB);
		//  XOR C
		//  1   4
		// Z 0 0 0
		case 0xA9: return XOR_r(RC);
		//  XOR D
		//  1   4
		// Z 0 0 0
		case 0xAA: return XOR_r(RD);
		//  XOR E
		//  1   4
		// Z 0 0 0
		case 0xAB: return XOR_r(RE);
		//  XOR H
		//  1   4
		// Z 0 0 0
		case 0xAC: return XOR_r(RH);
		//  XOR L
		//  1   4
		// Z 0 0 0
		case 0xAD: return XOR_r(RL);
		//  XOR (HL)
		//  1   8
		// Z 0 0 0
		case 0xAE: {
			if (m_instruction_remaining_cycles == 0)
			{
				m_bus.write_addr(RHL);

				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return XOR_r(m_bus.read_data());
			}

			return -2;
		};
		//  XOR A
		//  1   4
		// Z 0 0 0
		case 0xAF: return XOR_r(RA);

		//  OR B
		//  1   4
		// Z 0 0 0
		case 0xB0: return OR_r(RB);
		//  OR C
		//  1   4
		// Z 0 0 0
		case 0xB1: return OR_r(RC);
		//  OR D
		//  1   4
		// Z 0 0 0
		case 0xB2: return OR_r(RD);
		//  OR E
		//  1   4
		// Z 0 0 0
		case 0xB3: return OR_r(RE);
		//  OR H
		//  1   4
		// Z 0 0 0
		case 0xB4: return OR_r(RH);
		//  OR L
		//  1   4
		// Z 0 0 0
		case 0xB5: return OR_r(RL);
		// OR (HL)
		//  1   8
		// Z 0 0 0
		case 0xB6: {
			if (m_instruction_remaining_cycles == 0)
			{
				m_bus.write_addr(RHL);

				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return OR_r(m_bus.read_data());
			}

			return -2;
		};
		//  OR A
		//  1   4
		// Z 0 0 0
		case 0xB7: return OR_r(RA);

		//  CP B
		//  1   4
		// Z 1 H C
		case 0xB8: return CP_r(RB);
		//  CP C
		//  1   4
		// Z 1 H C
		case 0xB9: return CP_r(RC);
		//  CP D
		//  1   4
		// Z 1 H C
		case 0xBA: return CP_r(RD);
		//  CP E
		//  1   4
		// Z 1 H C
		case 0xBB: return CP_r(RE);
		//  CP H
		//  1   4
		// Z 1 H C
		case 0xBC: return CP_r(RH);
		//  CP L
		//  1   4
		// Z 1 H C
		case 0xBD: return CP_r(RL);
		//  CP (HL)
		//  1   8
		// Z 1 H C
		case 0xBE: {
			if (m_instruction_remaining_cycles == 0)
			{
				m_bus.write_addr(RHL);

				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return CP_r(m_bus.read_data());
			}

			return -2;
		};
		//  CP A
		//  1   4
		// Z 1 H C
		case 0xBF: return CP_r(RA);

		// ADD A,d8
		//  2   8
		// Z 0 H C
		case 0xC6: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				return ADD_r(m_bus.read_data());
			}

			return -2;
		};
		// ADC A,d8
		//  2   8
		// Z 0 H C
		case 0xCE: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				return ADC_r(m_bus.read_data());
			}

			return -2;
		};
		// SUB d8
		//  2   8
		// Z 1 H C
		case 0xD6: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				return SUB_r(m_bus.read_data());
			}

			return -2;
		};
		// SBC A,d8
		//  2   8
		// Z 1 H C
		case 0xDE: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				return SBC_r(m_bus.read_data());
			}

			return -2;
		};
		// AND d8
		//  2   8
		// Z 0 1 0
		case 0xE6: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				return AND_r(m_bus.read_data());
			}

			return -2;
		};
		// XOR d8
		// 	2   8
		// Z 0 0 0
		case 0xEE: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC);
				return 1;
			}

			if (m_instruction_remaining_cycles == 1)
			{
				return XOR_r(m_bus.read_data());
			}

			return -2;
		};
		//  OR d8
		//  2   8
		// Z 0 0 0
		case 0xF6: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RPC); 
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				return OR_r(m_bus.read_data());
			}

			return -2;
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
				return CP_r(m_bus.read_data());
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

		// JP NZ,a16
		// 3  16/12
		// - - - -
		case 0xC2: return JP_cc_nn(!FZ);
		// JP Z,a16
		// 3  16/12
		// - - - -
		case 0xCA: return JP_cc_nn(FZ);
		// JP NC,a16
		// 3  16/12
		// - - - -
		case 0xD2: return JP_cc_nn(!FC);
		// JP C,a16
		// 3  16/12
		// - - - -
		case 0xDA: return JP_cc_nn(FC);
		// JP a16
		//  3  16
		// - - - -
		case 0xC3: return JP_cc_nn(1);
		// JP (HL)
		//  1   4
		// - - - -
		case 0xE9: {
			RPC = RHL;
			return 0;
		};

		// RET NZ
		// 1  20/8
		// - - - -
		case 0xC0: return RET_cc(!FZ);
		// RET Z
		// 1  20/8
		// - - - -
		case 0xC8: return RET_cc(FZ);
		// RET NC
		// 1  20/8
		// - - - -
		case 0xD0: return RET_cc(!FC);
		// RET C
		// 1  20/8
		// - - - -
		case 0xD8: return RET_cc(FC);
		
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
				return 0;
			}
			return -2;
		};
		//  RETI
		//  1  16
		// - - - -
		case 0xD9: {
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
				// TODO: enable interrupts
				return 0;
			}
			return -2;
		};

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
				m_instruction_byte2 = lsb(RPC);
				set_lsb(RPC, m_instruction_byte1);
				m_instruction_byte1 = msb(RPC);
				set_msb(RPC, m_bus.read_data());
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
				return 0;
			}

			return -2;
		};
		// CALL NZ,a16
		// 3  24/12
		// - - - -
		case 0xC4: {
			if (FZ)
			{
				if (m_instruction_remaining_cycles == 0)
				{
					++RPC;
					m_bus.write_addr(RPC); 
					return 2;
				}

				if (m_instruction_remaining_cycles == 2)
				{
					++RPC;
					(void)m_bus.read_data();
					m_bus.write_addr(RPC); 
					return 1;
				}

				if (m_instruction_remaining_cycles == 1)
				{
					++RPC;
					(void)m_bus.read_data();
					return 0;
				}
			}
			else
			{
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
					m_instruction_byte2 = lsb(RPC);
					set_lsb(RPC, m_instruction_byte1);
					m_instruction_byte1 = msb(RPC);
					set_msb(RPC, m_bus.read_data());
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
					return 0;
				}
			}

			return -2;
		};

		// RST 00H
		//  1  16
		// - - - -
		case 0xC7: return RST_nn(0x00);
		// RST 08H
		//  1  16
		// - - - -
		case 0xCF: return RST_nn(0x08);
		// RST 10H
		//  1  16
		// - - - -
		case 0xD7: return RST_nn(0x10);
		// RST 18H
		//  1  16
		// - - - -
		case 0xDF: return RST_nn(0x18);
		// RST 20H
		//  1  16
		// - - - -
		case 0xE7: return RST_nn(0x20);
		// RST 28H
		//  1  16
		// - - - -
		case 0xEF: return RST_nn(0x28);
		// RST 30H
		//  1  16
		// - - - -
		case 0xF7: return RST_nn(0x30);
		// RST 38H
		//  1  16
		// - - - -
		case 0xFF: return RST_nn(0x38);

		// LD A,(BC)
		//  1   8
		// - - - -
		case 0x0A: return LD_r_rr_address(RA, RBC);
		// LD A,(DE)
		//  1   8
		// - - - -
		case 0x1A: return LD_r_rr_address(RA, RDE);
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
				return 0;
			}
			return -2;
		};
		// LD A,(HL-)
		//  1   8
		// - - - -
		case 0x3A: {
			if (m_instruction_remaining_cycles == 0)
			{
				++RPC;
				m_bus.write_addr(RHL); 
				--RHL;
				return 1;
			}
			
			if (m_instruction_remaining_cycles == 1)
			{
				RA = m_bus.read_data();
				return 0;
			}
			return -2;
		};

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

		// Misc/control instructions

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
		//  HALT
		//  1   4
		// - - - -
		case 0x76: {
			++RPC;
			m_stop = true;
			return 0;
		};
		
		// PREFIX CB
		//  1   4
		// - - - -
		case 0xCB: return execute_prefixed_instruction();
		//   DI
		//  1   4
		// - - - -
		case 0xF3: {
			++RPC;
			// printf("Interrupts not yet implemented.\n"); // TODO
		} break;
		//   EI
		//  1   4
		// - - - -
		case 0xFB: {
			++RPC;
			// printf("Interrupts not yet implemented.\n"); // TODO
		} break;

		case 0xD3:
		case 0xDB:
		case 0xDD:
		case 0xE3:
		case 0xE4:
		case 0xEB:
		case 0xEC:
		case 0xED:
		case 0xF4:
		case 0xFC:
		case 0xFD:
			++RPC;
			printf("Undefined opcode %#04x, ignoring for now.\n", m_instruction_byte0);
			return 0;

		default:
			if (s_instruction_names.contains(m_instruction_byte0))
				printf("Uninplemented instruction %#4x: %s\n", m_instruction_byte0, std::get<0>(s_instruction_names.at(m_instruction_byte0)).c_str());
			else
				printf("Uninplemented instruction %#4x\n", m_instruction_byte0);

			// halt cpu
			return -2;
	}


	return 0;
}

int8_t Cpu::execute_prefixed_instruction()
{
	if (m_instruction_remaining_cycles == 0)
	{
		++RPC;
		m_bus.write_addr(RPC);
		return 1;
	}

	if (m_instruction_remaining_cycles == 1)
		m_instruction_byte1 = m_bus.read_data();

	static auto const RLC_r = [&](uint8_t& r) -> int8_t {
		++RPC;
		FN = 0;
		FH = 0;
		FC = r & 0b10000000;

		r = ((r << 1) & 0x11111110) | ((r >> 7) & 0b00000001);
		FZ = (r == 0);

		return 0;
	};
	static auto const RRC_r = [&](uint8_t& r) -> int8_t {
		++RPC;
		FN = 0;
		FH = 0;
		FC = r & 0b00000001;

		r = ((r >> 1) & 0x01111111) | ((r << 7) & 0b10000000);
		FZ = (r == 0);

		return 0;
	};
	static auto const RL_r = [&](uint8_t& r) -> int8_t {
		++RPC;
		FN = 0;
		FH = 0;
		uint8_t FT = FC;
		FC = (r & 0b10000000);

		r <<= 1;
		r &= 0b11111110;
		r |= FT & 0b00000001;
		
		FZ = (r == 0);
		return 0;
	};
	static auto const RR_r = [&](uint8_t& r) -> int8_t {
		++RPC;
		FN = 0;
		FH = 0;
		uint8_t FT = FC;
		FC = (r & 0b00000001);

		r >>= 1;
		r &= 0b01111111;
		r |= (FT << 7) & 0b10000000;
		
		FZ = (r == 0);
		return 0;
	};
	static auto const SLA_r = [&](uint8_t& r) -> int8_t {
		++RPC;
		FN = 0;
		FH = 0;
		FC = (r & 0b10000000);

		r <<= 1;
		r &= 0b11111110;

		FZ = (r == 0);
		
		return 0;
	};
	static auto const SRA_r = [&](uint8_t& r) -> int8_t {
		++RPC;
		FN = 0;
		FH = 0;
		FC = 0;

		uint8_t RT = r & 0b10000000;

		r >>= 1;
		r &= 0b01111111;
		r |= RT;

		FZ = (r == 0);
		
		return 0;
	};
	static auto const SWAP_r = [&](uint8_t& r) -> int8_t {
		++RPC;
		FN = 0;
		FH = 0;
		FC = 0;

		r = ((r & 0x0F) << 4) | ((r & 0xF0) >> 4);
		
		FZ = (r == 0);
		return 0;
	};
	static auto const SRL_r = [&](uint8_t& r) -> int8_t {
		++RPC;
		FN = 0;
		FH = 0;
		FC = (r & 0b00000001);

		r >>= 1;
		r &= 0b01111111;
		
		FZ = (r == 0);
		return 0;
	};

	// static auto const _r = [&](uint8_t& r) -> int8_t {
	// 	++RPC;
	// 	// FZ = ?;
	// 	// FN = ?;
	// 	// FH = ?;
	// 	// FC = ?;

	// 	// r = ?;
		
	// 	return 0;
	// };

	switch (m_instruction_byte1)
	{
		//  RLC B
		//  2   8
		// Z 0 0 C
		case 0x00: return RLC_r(RB);
		//  RLC C
		//  2   8
		// Z 0 0 C
		case 0x01: return RLC_r(RC);
		//  RLC D
		//  2   8
		// Z 0 0 C
		case 0x02: return RLC_r(RD);
		//  RLC E
		//  2   8
		// Z 0 0 C
		case 0x03: return RLC_r(RE);
		//  RLC H
		//  2   8
		// Z 0 0 C
		case 0x04: return RLC_r(RH);
		//  RLC L
		//  2   8
		// Z 0 0 C
		case 0x05: return RLC_r(RL);
		// RLC (HL)
		//  2   16
		// Z 0 0 C
		case 0x06: {
			if (m_instruction_remaining_cycles == 1)
			{
				m_bus.write_addr(RHL);
				return 3;
			}
			
			if (m_instruction_remaining_cycles == 3)
			{
				uint8_t RT = m_bus.read_data();
				(void)RLC_r(RT);
				m_bus.write_addr(RHL);
				m_bus.write_data(RT);
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				return 0;
			}

			return -2;
		};
		//  RLC A
		//  2   8
		// Z 0 0 C
		case 0x07: return RLC_r(RA);
		
		//  RRC B
		//  2   8
		// Z 0 0 C
		case 0x08: return RRC_r(RB);
		//  RRC C
		//  2   8
		// Z 0 0 C
		case 0x09: return RRC_r(RC);
		//  RRC D
		//  2   8
		// Z 0 0 C
		case 0x0A: return RRC_r(RD);
		//  RRC E
		//  2   8
		// Z 0 0 C
		case 0x0B: return RRC_r(RE);
		//  RRC H
		//  2   8
		// Z 0 0 C
		case 0x0C: return RRC_r(RH);
		//  RRC L
		//  2   8
		// Z 0 0 C
		case 0x0D: return RRC_r(RL);
		// RRC (HL)
		//  2   16
		// Z 0 0 C
		case 0x0E: {
			if (m_instruction_remaining_cycles == 1)
			{
				m_bus.write_addr(RHL);
				return 3;
			}
			
			if (m_instruction_remaining_cycles == 3)
			{
				uint8_t RT = m_bus.read_data();
				(void)RRC_r(RT);
				m_bus.write_addr(RHL);
				m_bus.write_data(RT);
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				return 0;
			}

			return -2;
		};
		//  RRC A
		//  2   8
		// Z 0 0 C
		case 0x0F: return RRC_r(RA);
		
		//  RL B
		//  2   8
		// Z 0 0 C
		case 0x10: return RL_r(RB);
		//  RL C
		//  2   8
		// Z 0 0 C
		case 0x11: return RL_r(RC);
		//  RL D
		//  2   8
		// Z 0 0 C
		case 0x12: return RL_r(RD);
		//  RL E
		//  2   8
		// Z 0 0 C
		case 0x13: return RL_r(RE);
		//  RL H
		//  2   8
		// Z 0 0 C
		case 0x14: return RL_r(RH);
		//  RL L
		//  2   8
		// Z 0 0 C
		case 0x15: return RL_r(RL);
		// RL (HL)
		//  2   16
		// Z 0 0 C
		case 0x16: {
			if (m_instruction_remaining_cycles == 1)
			{
				m_bus.write_addr(RHL);
				return 3;
			}
			
			if (m_instruction_remaining_cycles == 3)
			{
				uint8_t RT = m_bus.read_data();
				(void)RL_r(RT);
				m_bus.write_addr(RHL);
				m_bus.write_data(RT);
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				return 0;
			}

			return -2;
		};
		//  RL A
		//  2   8
		// Z 0 0 C
		case 0x17: return RL_r(RA);

		//  RR B
		//  2   8
		// Z 0 0 C
		case 0x18: return RR_r(RB);
		//  RR C
		//  2   8
		// Z 0 0 C
		case 0x19: return RR_r(RC);
		//  RR D
		//  2   8
		// Z 0 0 C
		case 0x1A: return RR_r(RD);
		//  RR E
		//  2   8
		// Z 0 0 C
		case 0x1B: return RR_r(RE);
		//  RR H
		//  2   8
		// Z 0 0 C
		case 0x1C: return RR_r(RH);
		//  RR L
		//  2   8
		// Z 0 0 C
		case 0x1D: return RR_r(RL);
		//  RR (HL)
		//  2   16
		// Z 0 0 C
		case 0x1E: {
			if (m_instruction_remaining_cycles == 1)
			{
				m_bus.write_addr(RHL);
				return 3;
			}
			
			if (m_instruction_remaining_cycles == 3)
			{
				uint8_t RT = m_bus.read_data();
				(void)RR_r(RT);
				m_bus.write_addr(RHL);
				m_bus.write_data(RT);
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				return 0;
			}

			return -2;
		};
		//  RR A
		//  2   8
		// Z 0 0 C
		case 0x1F: return RR_r(RA);
		
		//  SLA B
		//  2   8
		// Z 0 0 C
		case 0x20: return SLA_r(RB);
		//  SLA C
		//  2   8
		// Z 0 0 C
		case 0x21: return SLA_r(RC);
		//  SLA D
		//  2   8
		// Z 0 0 C
		case 0x22: return SLA_r(RD);
		//  SLA E
		//  2   8
		// Z 0 0 C
		case 0x23: return SLA_r(RE);
		//  SLA H
		//  2   8
		// Z 0 0 C
		case 0x24: return SLA_r(RH);
		//  SLA L
		//  2   8
		// Z 0 0 C
		case 0x25: return SLA_r(RL);
		// SLA (HL)
		//  2   16
		// Z 0 0 C
		case 0x26: {
			if (m_instruction_remaining_cycles == 1)
			{
				m_bus.write_addr(RHL);
				return 3;
			}
			
			if (m_instruction_remaining_cycles == 3)
			{
				uint8_t RT = m_bus.read_data();
				(void)SLA_r(RT);
				m_bus.write_addr(RHL);
				m_bus.write_data(RT);
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				return 0;
			}

			return -2;
		};
		//  SLA A
		//  2   8
		// Z 0 0 C
		case 0x27: return SLA_r(RA);

		//  SRA B
		//  2   8
		// Z 0 0 0
		case 0x28: return SRA_r(RB);
		//  SRA C
		//  2   8
		// Z 0 0 0
		case 0x29: return SRA_r(RC);
		//  SRA D
		//  2   8
		// Z 0 0 0
		case 0x2A: return SRA_r(RD);
		//  SRA E
		//  2   8
		// Z 0 0 0
		case 0x2B: return SRA_r(RE);
		//  SRA H
		//  2   8
		// Z 0 0 0
		case 0x2C: return SRA_r(RH);
		//  SRA L
		//  2   8
		// Z 0 0 0
		case 0x2D: return SRA_r(RL);
		//  SRA (HL)
		//  2   16
		// Z 0 0 0
		case 0x2E: {
			if (m_instruction_remaining_cycles == 1)
			{
				m_bus.write_addr(RHL);
				return 3;
			}
			
			if (m_instruction_remaining_cycles == 3)
			{
				uint8_t RT = m_bus.read_data();
				(void)SRA_r(RT);
				m_bus.write_addr(RHL);
				m_bus.write_data(RT);
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				return 0;
			}

			return -2;
		};
		//  SRA A
		//  2   8
		// Z 0 0 0
		case 0x2F: return SRA_r(RA);

		// SWAP B
		//  2   8
		// Z 0 0 0
		case 0x30: return SWAP_r(RB);
		// SWAP C
		//  2   8
		// Z 0 0 0
		case 0x31: return SWAP_r(RC);
		// SWAP D
		//  2   8
		// Z 0 0 0
		case 0x32: return SWAP_r(RD);
		// SWAP E
		//  2   8
		// Z 0 0 0
		case 0x33: return SWAP_r(RE);
		// SWAP H
		//  2   8
		// Z 0 0 0
		case 0x34: return SWAP_r(RH);
		// SWAP L
		//  2   8
		// Z 0 0 0
		case 0x35: return SWAP_r(RL);
		// SWAP (HL)
		//  2   16
		// Z 0 0 0
		case 0x36: {
			if (m_instruction_remaining_cycles == 1)
			{
				m_bus.write_addr(RHL);
				return 3;
			}
			
			if (m_instruction_remaining_cycles == 3)
			{
				uint8_t RT = m_bus.read_data();
				(void)SWAP_r(RT);
				m_bus.write_addr(RHL);
				m_bus.write_data(RT);
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				return 0;
			}

			return -2;
		};
		// SWAP A
		//  2   8
		// Z 0 0 0
		case 0x37: return SWAP_r(RA);

		//  SRL B
		//  2   8
		// Z 0 0 C
		case 0x38: return SRL_r(RB);
		//  SRL C
		//  2   8
		// Z 0 0 C
		case 0x39: return SRL_r(RC);
		//  SRL D
		//  2   8
		// Z 0 0 C
		case 0x3A: return SRL_r(RD);
		//  SRL E
		//  2   8
		// Z 0 0 C
		case 0x3B: return SRL_r(RE);
		//  SRL H
		//  2   8
		// Z 0 0 C
		case 0x3C: return SRL_r(RH);
		//  SRL L
		//  2   8
		// Z 0 0 C
		case 0x3D: return SRL_r(RL);
		//  SRL (HL)
		//  2   16
		// Z 0 0 C
		case 0x3E: {
			if (m_instruction_remaining_cycles == 1)
			{
				m_bus.write_addr(RHL);
				return 3;
			}
			
			if (m_instruction_remaining_cycles == 3)
			{
				uint8_t RT = m_bus.read_data();
				(void)SRL_r(RT);
				m_bus.write_addr(RHL);
				m_bus.write_data(RT);
				return 2;
			}

			if (m_instruction_remaining_cycles == 2)
			{
				return 0;
			}

			return -2;
		};
		//  SRL A
		//  2   8
		// Z 0 0 C
		case 0x3F: return SRL_r(RA);
		default:
			printf("Uninplemented instruction with prefix %#x: %s\n", m_instruction_byte1, s_prefixed_instruction_names[m_instruction_byte1]);
			return -2;
	}

	return -2;
}
