#pragma once
#include <cstdint>
#include <string>

class Chip8 {
public:
	Chip8();
	void LoadROM(const std::string& filename);
	void Cycle();          // Fetch, Decode, Execute (700Hz)
	void Tick();           // Timers, draw and timer opcodes (60Hz)

	static const unsigned int VIDEO_WIDTH = 64;
	static const unsigned int VIDEO_HEIGHT = 32;

	uint8_t video[VIDEO_WIDTH * VIDEO_HEIGHT]; // Display memory
	uint8_t keypad[16];                        // Current keypad state
	uint8_t previousKeypad[16];                // Previous keypad state
	bool drawFlag = false;                     // Set when screen needs redraw

private:
	uint16_t opcode;
	uint8_t memory[4096];
	uint8_t V[16];
	uint16_t I;
	uint16_t pc;

	uint16_t stack[16];
	uint8_t sp;

	uint8_t delayTimer;
	uint8_t soundTimer;

	void ExecuteOpcode(uint16_t opcode);
};
