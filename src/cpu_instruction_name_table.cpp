#include "cpu.h"

// https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html

std::map<uint8_t, std::tuple<std::string, int8_t>> const Cpu::s_instruction_names = std::map<uint8_t, std::tuple<std::string, int8_t>> {
	{ 0x00, { "NOP", 1 } },
	{ 0x01, { "LD BC,d16", 3 } },
	{ 0x02, { "LD (BC),A", 1 } },
	{ 0x03, { "INC BC", 1 } },
	{ 0x04, { "INC B", 1 } },
	{ 0x05, { "DEC B", 1 } },
	{ 0x06, { "LD B,d8", 2 } },
	{ 0x07, { "RLCA", 1 } },
	{ 0x09, { "ADD HL,BC", 1 } },
	{ 0x0A, { "LD A,(BC)", 1} },
	{ 0x0B, { "DEC BC", 1 } },
	{ 0x0C, { "INC C", 1 } },
	{ 0x0D, { "DEC C", 1 } },
	{ 0x0E, { "LD C,d8", 2 } },
	{ 0x0F, { "RRCA", 1 } },
	{ 0x10, { "STOP 0", 2 } },
	{ 0x11, { "LD DE,d16", 3 } },
	{ 0x12, { "LD (DE),A", 1 } },
	{ 0x13, { "INC DE", 1 } },
	{ 0x14, { "INC D", 1 } },
	{ 0x15, { "DEC D", 1 } },
	{ 0x16, { "LD D,d8", 2 } },
	{ 0x17, { "RLA", 1 } },
	{ 0x18, { "JR r8", 2 } },
	{ 0x19, { "ADD HL,DE", 1 } },
	{ 0x1A, { "LD A,(DE)", 1} },
	{ 0x1B, { "DEC DE", 1 } },
	{ 0x1C, { "INC E", 1 } },
	{ 0x1D, { "DEC E", 1 } },
	{ 0x1E, { "LD E,d8", 2 } },
	{ 0x1F, { "RRA", 1 } },
	{ 0x20, { "JR NZ,r8", 2 } },
	{ 0x21, { "LD HL,d16", 3 } },
	{ 0x22, { "LD (HL+),A", 1 } },
	{ 0x23, { "INC HL", 1 } },
	{ 0x24, { "INC H", 1 } },
	{ 0x25, { "DEC H", 1 } },
	{ 0x26, { "LD H,d8", 2 } },
	{ 0x27, { "DAA", 1 } },
	{ 0x28, { "JR Z,r8", 2 } },
	{ 0x29, { "ADD HL,HL", 1 } },
	{ 0x2A, { "LD A,(HL+)", 1} },
	{ 0x2B, { "DEC HL", 1 } },
	{ 0x2C, { "INC L", 1 } },
	{ 0x2D, { "DEC L", 1 } },
	{ 0x2E, { "LD L,d8", 2 } },
	{ 0x2F, { "CPL", 1 } },
	{ 0x30, { "JR NC,r8", 2 } },
	{ 0x31, { "LD SP,d16", 3 } },
	{ 0x32, { "LD (HL-),A", 1 } },
	{ 0x33, { "INC SP", 1 } },
	{ 0x34, { "INC (HL)", 1 } },
	{ 0x35, { "DEC (HL)", 1 } },
	{ 0x36, { "LD (HL),d8", 2 } },
	{ 0x37, { "SCF", 1 } },
	{ 0x38, { "JR C,r8", 2 } }, 
	{ 0x39, { "ADD HL,SP", 1 } },
	{ 0x3A, { "LD A,(HL-)", 1} },
	{ 0x3B, { "DEC SP", 1 } },
	{ 0x3C, { "INC A", 1 } },
	{ 0x3D, { "DEC A", 1 } },
	{ 0x3E, { "LD A,d8", 2 } },
	{ 0x3F, { "CCF", 1 } },
	{ 0x40, { "LD B,B", 1 } },
	{ 0x41, { "LD B,C", 1 } },
	{ 0x42, { "LD B,D", 1 } },
	{ 0x43, { "LD B,E", 1 } },
	{ 0x44, { "LD B,H", 1 } },
	{ 0x45, { "LD B,L", 1 } },
	{ 0x46, { "LD B,(HL)", 1 } },
	{ 0x47, { "LD B,A", 1 } },
	{ 0x48, { "LD C,B", 1 } },
	{ 0x49, { "LD C,C", 1 } },
	{ 0x4A, { "LD C,D", 1 } },
	{ 0x4B, { "LD C,E", 1 } },
	{ 0x4C, { "LD C,H", 1 } },
	{ 0x4D, { "LD C,L", 1 } },
	{ 0x4E, { "LD C,(HL)", 1 } },
	{ 0x4F, { "LD C,A", 1 } },
	{ 0x50, { "LD D,B", 1 } },
	{ 0x51, { "LD D,C", 1 } },
	{ 0x52, { "LD D,D", 1 } },
	{ 0x53, { "LD D,E", 1 } },
	{ 0x54, { "LD D,H", 1 } },
	{ 0x55, { "LD D,L", 1 } },
	{ 0x56, { "LD D,(HL)", 1 } },
	{ 0x57, { "LD D,A", 1 } },
	{ 0x58, { "LD E,B", 1 } },
	{ 0x59, { "LD E,C", 1 } },
	{ 0x5A, { "LD E,D", 1 } },
	{ 0x5B, { "LD E,E", 1 } },
	{ 0x5C, { "LD E,H", 1 } },
	{ 0x5D, { "LD E,L", 1 } },
	{ 0x5E, { "LD E,(HL)", 1 } },
	{ 0x5F, { "LD E,A", 1 } },
	{ 0x60, { "LD H,B", 1 } },
	{ 0x61, { "LD H,C", 1 } },
	{ 0x62, { "LD H,D", 1 } },
	{ 0x63, { "LD H,E", 1 } },
	{ 0x64, { "LD H,H", 1 } },
	{ 0x65, { "LD H,L", 1 } },
	{ 0x66, { "LD H,(HL)", 1 } },
	{ 0x67, { "LD H,A", 1 } },
	{ 0x68, { "LD L,B", 1 } },
	{ 0x69, { "LD L,C", 1 } },
	{ 0x6A, { "LD L,D", 1 } },
	{ 0x6B, { "LD L,E", 1 } },
	{ 0x6C, { "LD L,H", 1 } },
	{ 0x6D, { "LD L,L", 1 } },
	{ 0x6E, { "LD L,(HL)", 1 } },
	{ 0x6F, { "LD L,A", 1 } },
	{ 0x70, { "LD (HL),B", 1 } },
	{ 0x71, { "LD (HL),C", 1 } },
	{ 0x72, { "LD (HL),D", 1 } },
	{ 0x73, { "LD (HL),E", 1 } },
	{ 0x74, { "LD (HL),H", 1 } },
	{ 0x75, { "LD (HL),L", 1 } },
	{ 0x76, { "HALT", 1 } },
	{ 0x77, { "LD (HL),A", 1 } },
	{ 0x78, { "LD A,B", 1 } },
	{ 0x79, { "LD A,C", 1 } },
	{ 0x7A, { "LD A,D", 1 } },
	{ 0x7B, { "LD A,E", 1 } },
	{ 0x7C, { "LD A,H", 1 } },
	{ 0x7D, { "LD A,L", 1 } },
	{ 0x7E, { "LD A,(HL)", 1 } },
	{ 0x7F, { "LD A,A", 1 } },
	{ 0x80, { "ADD A,B", 1 } },
	{ 0x81, { "ADD A,C", 1 } },
	{ 0x82, { "ADD A,D", 1 } },
	{ 0x83, { "ADD A,E", 1 } },
	{ 0x84, { "ADD A,H", 1 } },
	{ 0x85, { "ADD A,L", 1 } },
	{ 0x86, { "ADD A,(HL)", 1 } },
	{ 0x87, { "ADD A,A", 1 } },
	{ 0x88, { "ADC A,B", 1 } },
	{ 0x89, { "ADC A,C", 1 } },
	{ 0x8A, { "ADC A,D", 1 } },
	{ 0x8B, { "ADC A,E", 1 } },
	{ 0x8C, { "ADC A,H", 1 } },
	{ 0x8D, { "ADC A,L", 1 } },
	{ 0x8E, { "ADC A,(HL)", 1 } },
	{ 0x8F, { "ADC A,A", 1 } },
	{ 0x90, { "SUB B", 1 } },
	{ 0x91, { "SUB C", 1 } },
	{ 0x92, { "SUB D", 1 } },
	{ 0x93, { "SUB E", 1 } },
	{ 0x94, { "SUB H", 1 } },
	{ 0x95, { "SUB L", 1 } },
	{ 0x96, { "SUB (HL)", 1 } },
	{ 0x97, { "SUB A", 1 } },
	{ 0x98, { "SBC A,B", 1 } },
	{ 0x99, { "SBC A,C", 1 } },
	{ 0x9A, { "SBC A,D", 1 } },
	{ 0x9B, { "SBC A,E", 1 } },
	{ 0x9C, { "SBC A,H", 1 } },
	{ 0x9D, { "SBC A,L", 1 } },
	{ 0x9E, { "SBC A,(HL)", 1 } },
	{ 0x9F, { "SBC A,A", 1 } },
	{ 0xA8, { "XOR B", 1 } },
	{ 0xA9, { "XOR C", 1 } },
	{ 0xAA, { "XOR D", 1 } },
	{ 0xAB, { "XOR E", 1 } },
	{ 0xAC, { "XOR H", 1 } },
	{ 0xAD, { "XOR L", 1 } },
	{ 0xAE, { "XOR (HL)", 1 } },
	{ 0xAF, { "XOR A", 1 } },
	{ 0xB0, { "OR B", 1 } },
	{ 0xB1, { "OR C", 1 } },
	{ 0xB2, { "OR D", 1 } },
	{ 0xB3, { "OR E", 1 } },
	{ 0xB4, { "OR H", 1 } },
	{ 0xB5, { "OR L", 1 } },
	{ 0xB6, { "OR (HL)", 1 } },
	{ 0xB7, { "OR A", 1 } },
	{ 0xB8, { "CP B", 1 } },
	{ 0xB9, { "CP C", 1 } },
	{ 0xBA, { "CP D", 1 } },
	{ 0xBB, { "CP E", 1 } },
	{ 0xBC, { "CP H", 1 } },
	{ 0xBD, { "CP L", 1 } },
	{ 0xBE, { "CP (HL)", 1 } },
	{ 0xBF, { "CP A", 1 } },
	{ 0xC0, { "RET NZ", 1 } },
	{ 0xC1, { "POP BC", 1 } },
	{ 0xC3, { "JP a16", 3 } },
	{ 0xC2, { "JP NZ,a16", 3 } },
	{ 0xC4, { "CALL NZ,a16", 3 } },
	{ 0xC5, { "PUSH BC", 1 } },
	{ 0xC6, { "ADD A,d8", 1 } },
	{ 0xC7, { "RST 00H", 1 } },
	{ 0xC8, { "RET Z", 1 } },
	{ 0xC9, { "RET", 1 } },
	{ 0xCA, { "JP Z,a16", 3 } },
	{ 0xCB, { "PREFIX CB", 2 } },
	{ 0xCD, { "CALL a16", 3 } },
	{ 0xCE, { "ADC A,d8", 2 } },
	{ 0xCF, { "RST 08H", 1 } },
	{ 0xD0, { "RET NC", 1 } },
	{ 0xD1, { "POP DE", 1 } },
	{ 0xD2, { "JP NC,a16", 3 } },
	{ 0xD3, { "UNDEFINED D3", 1 } },
	{ 0xD5, { "PUSH DE", 1 } },
	{ 0xD6, { "SUB d8", 2 } },
	{ 0xD7, { "RST 10H", 1 } },
	{ 0xD8, { "RET C", 1 } },
	{ 0xDA, { "JP C,a16", 3 } },
	{ 0xDB, { "UNDEFINED DB", 1 } },
	{ 0xDD, { "UNDEFINED DD", 1 } },
	{ 0xDE, { "SBC A,d8", 2 } },
	{ 0xDF, { "RST 18H", 1 } },
	{ 0xE0, { "LDH (a8),A", 2 } },
	{ 0xE1, { "POP HL", 1 } },
	{ 0xE3, { "UNDEFINED E3", 1 } },
	{ 0xE4, { "UNDEFINED E4", 1 } },
	{ 0xE5, { "PUSH HL", 1 } },
	{ 0xE6, { "AND d8", 2 } },
	{ 0xE7, { "RST 20H", 1 } },
	{ 0xE9, { "JP (HL)", 1 } },
	{ 0xEA, { "LD (a16),A", 3 } },
	{ 0xEB, { "UNDEFINED EB", 1 } },
	{ 0xEC, { "UNDEFINED EC", 1 } },
	{ 0xED, { "UNDEFINED ED", 1 } },
	{ 0xEE, { "XOR d8", 2 } },
	{ 0xEF, { "RST 28H", 1 } },
	{ 0xF0, { "LDH A,(a8)", 2 } },
	{ 0xF3, { "DI", 1 } },
	{ 0xF1, { "POP AF", 1 } },
	{ 0xF4, { "UNDEFINED F4", 1 } },
	{ 0xF5, { "PUSH AF", 1 } },
	{ 0xF6, { "OR d8", 1 } },
	{ 0xF7, { "RST 30H", 1 } },
	{ 0xFA, { "LD A,(a16)", 3 } },
	{ 0xFB, { "EI", 1 } },
	{ 0xFC, { "UNDEFINED FC", 1 } },
	{ 0xFD, { "UNDEFINED FD", 1 } },
	{ 0xFE, { "CP d8", 1 } },
	{ 0xFF, { "RST 38H", 1 } },
};