#pragma once
#include <cstdint>

class Bus
{
public:
	uint8_t read_data();
	void write_data(uint8_t data);
	bool mem_data_ready() const {
		return m_mem_data_ready;
	};
	void mem_did_read_data() {
		m_mem_data_ready = false;
	};

	uint16_t read_addr();
	void write_addr(uint16_t addr);

	Bus() = default;
	~Bus() = default;

private:
	uint8_t m_data;
	bool m_mem_data_ready = false;
	uint16_t m_addr;
};