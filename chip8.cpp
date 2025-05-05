#include "chip8.h"
#include <fstream>
#include <iostream>
#include <random>
#include <cstring>

// Standard CHIP8 fontset 16 characters × 5 bytes, loaded at 0x50
static const uint8_t chip8Fontset[80] = {
	0xF0,0x90,0x90,0x90,0xF0, // 0
	0x20,0x60,0x20,0x20,0x70, // 1
	0xF0,0x10,0xF0,0x80,0xF0, // 2
	0xF0,0x10,0xF0,0x10,0xF0, // 3
	0x90,0x90,0xF0,0x10,0x10, // 4
	0xF0,0x80,0xF0,0x10,0xF0, // 5
	0xF0,0x80,0xF0,0x90,0xF0, // 6
	0xF0,0x10,0x20,0x40,0x40, // 7
	0xF0,0x90,0xF0,0x90,0xF0, // 8
	0xF0,0x90,0xF0,0x10,0xF0, // 9
	0xF0,0x90,0xF0,0x90,0x90, // A
	0xE0,0x90,0xE0,0x90,0xE0, // B
	0xF0,0x80,0x80,0x80,0xF0, // C
	0xE0,0x90,0x90,0x90,0xE0, // D
	0xF0,0x80,0xF0,0x80,0xF0, // E
	0xF0,0x80,0xF0,0x80,0x80  // F
};

Chip8::Chip8() {
	pc = 0x200;
	opcode = 0;
	I = 0;
	sp = 0;

	// Clear memory, registers, display, stack, keypad
	memset(memory, 0, sizeof(memory));
	memset(V, 0, sizeof(V));
	memset(stack, 0, sizeof(stack));
	memset(video, 0, sizeof(video));
	memset(keypad, 0, sizeof(keypad));
	memset(previousKeypad, 0, sizeof(previousKeypad));

	// Load fontset at 0x50
	memcpy(memory + 0x50, chip8Fontset, sizeof(chip8Fontset));

	delayTimer = 0;
	soundTimer = 0;
	drawFlag = false;
}

void Chip8::LoadROM(const std::string& filename) {
	std::ifstream rom(filename, std::ios::binary | std::ios::ate);
	if (!rom) {
		std::cerr << "Failed to open ROM\n";
		return;
	}
	std::streamsize size = rom.tellg();
	rom.seekg(0, std::ios::beg);
	if (size + 0x200 > 4096) {
		std::cerr << "ROM too big for memory\n";
		return;
	}
	rom.read(reinterpret_cast<char*>(memory + 0x200), size);
}

void Chip8::Cycle() {
	opcode = memory[pc] << 8 | memory[pc + 1];
	ExecuteOpcode(opcode);
}

void Chip8::Tick() {
	if (delayTimer > 0) --delayTimer;
	if (soundTimer > 0) --soundTimer;
}

void Chip8::ExecuteOpcode(uint16_t opcode) {
	switch (opcode & 0xF000) {

	case 0x0000:
		switch (opcode & 0x00FF) {
		case 0x00E0: 
			memset(video, 0, sizeof(video));
			drawFlag = true;
			pc += 2;
			break;
		case 0x00EE: 
			--sp;
			pc = stack[sp];
			pc += 2;
			break;
		}
		break;

	case 0x1000: 
		pc = opcode & 0x0FFF;
		break;

	case 0x2000: 
		stack[sp++] = pc;
		pc = opcode & 0x0FFF;
		break;

	case 0x3000: 
		pc += (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) ? 4 : 2;
		break;

	case 0x4000: 
		pc += (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) ? 4 : 2;
		break;

	case 0x6000: 
		V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
		pc += 2;
		break;

	case 0x7000: 
		V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
		pc += 2;
		break;

	case 0x8000: {
		uint8_t x = (opcode & 0x0F00) >> 8;
		uint8_t y = (opcode & 0x00F0) >> 4;
		switch (opcode & 0x000F) {
		case 0x0: V[x] = V[y]; break;
		case 0x1: V[x] |= V[y]; break;
		case 0x2: V[x] &= V[y]; break;
		case 0x3: V[x] ^= V[y]; break;
		case 0x4: {
			uint16_t sum = V[x] + V[y];
			V[0xF] = sum > 255;
			V[x] = sum & 0xFF;
		} break;
		case 0x5:
			V[0xF] = V[x] > V[y];
			V[x] -= V[y];
			break;
		case 0x6:
			V[0xF] = V[x] & 1;
			V[x] >>= 1;
			break;
		case 0x7:
			V[0xF] = V[y] > V[x];
			V[x] = V[y] - V[x];
			break;
		case 0xE:
			V[0xF] = V[x] >> 7;
			V[x] <<= 1;
			break;
		}
		pc += 2;
	} break;

	case 0x9000: // SNE Vx, Vy
		pc += (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) ? 4 : 2;
		break;

	case 0xA000: // LD I, addr
		I = opcode & 0x0FFF;
		pc += 2;
		break;

	case 0xC000: // RND Vx, byte
		V[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
		pc += 2;
		break;

	case 0xD000: {  // Draw wrap horizontally, no vertical wrap
		uint8_t x = V[(opcode & 0x0F00) >> 8];
		uint8_t y = V[(opcode & 0x00F0) >> 4];
		uint8_t height = opcode & 0x000F;

		V[0xF] = 0;
		for (int row = 0; row < height; row++) {
			uint8_t sprite = memory[I + row];
			for (int col = 0; col < 8; col++) {
				if (!(sprite & (0x80 >> col))) continue;
				int px = (x + col) % VIDEO_WIDTH;
				int py = y + row;
				// skip pixels off top/bottom
				if (py < 0 || py >= VIDEO_HEIGHT) continue;
				int idx = py * VIDEO_WIDTH + px;
				if (video[idx]) V[0xF] = 1;
				video[idx] ^= 1;
			}
		}
		drawFlag = true;
		pc += 2;
	} break;

	case 0xE000:
		switch (opcode & 0x00FF) {
		case 0x9E: // SKP Vx
			pc += keypad[V[(opcode & 0x0F00) >> 8]] ? 4 : 2;
			break;
		case 0xA1: // SKNP Vx
			pc += !keypad[V[(opcode & 0x0F00) >> 8]] ? 4 : 2;
			break;
		}
		break;

	case 0xF000:
		switch (opcode & 0x00FF) {
		case 0x07: // LD Vx, DT
			V[(opcode & 0x0F00) >> 8] = delayTimer;
			pc += 2;
			break;
		case 0x15: // LD DT, Vx
			delayTimer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x18: // LD ST, Vx
			soundTimer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x1E: // ADD I, Vx
			I += V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x0A: { // LD Vx, K
			bool keyPress = false;
			for (int i = 0; i < 16; ++i) {
				if (keypad[i] && !previousKeypad[i]) {
					V[(opcode & 0x0F00) >> 8] = i;
					keyPress = true;
					break;
				}
			}
			if (!keyPress) return;  // wait
			pc += 2;
		} break;
		case 0x29: // LD F, Vx
			I = V[(opcode & 0x0F00) >> 8] * 5;
			pc += 2;
			break;
		case 0x33: { // LD B, Vx
			uint8_t vx = V[(opcode & 0x0F00) >> 8];
			memory[I] = vx / 100;
			memory[I + 1] = (vx / 10) % 10;
			memory[I + 2] = vx % 10;
			pc += 2;
		} break;
		case 0x55: // LD [I], Vx
			for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
				memory[I + i] = V[i];
			pc += 2;
			break;
		case 0x65: // LD Vx, [I]
			for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
				V[i] = memory[I + i];
			pc += 2;
			break;
		}
		break;

	default:
		std::cout << "Unknown opcode: " << std::hex << opcode << std::endl;
		pc += 2;
		break;
	}
}
