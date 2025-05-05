#include "chip8.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <iostream>

// File picker 
std::string OpenFileDialog() {
	char filename[MAX_PATH] = "";
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = "CHIP-8 ROMs\0*.ch8\0All Files\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = "ch8";

	if (GetOpenFileName(&ofn))
		return std::string(filename);
	else
		return "";
}

int main() {
	Chip8 chip8;
	std::string romPath = OpenFileDialog();
	if (romPath.empty()) return 0;
	chip8.LoadROM(romPath);

	// Create window and pixel block
	sf::RenderWindow window(
		sf::VideoMode(Chip8::VIDEO_WIDTH * 10, Chip8::VIDEO_HEIGHT * 10),
		"CHIP-8 Emulator"
	);
	sf::RectangleShape pixel(sf::Vector2f(10, 10));

	// Timers and frequencies
	sf::Clock clock;
	float cpuTimer = 0.0f;
	float timerTimer = 0.0f;
	const float cycleDelay = 1.0f / 500.0f;   // ~500 Hz CPU
	const float timerDelay = 1.0f / 60.0f;    // 60 Hz timers

	while (window.isOpen()) {
		// Poll events 
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// Delta time update 
		float deltaTime = clock.restart().asSeconds();
		cpuTimer += deltaTime;
		timerTimer += deltaTime;

		// CPU cycles at 500Hz
		while (cpuTimer >= cycleDelay) {
			chip8.Cycle();
			cpuTimer -= cycleDelay;
		}

		// Timers and input at 60Hz
		if (timerTimer >= timerDelay) {
			uint8_t newKeypad[16] = {
				sf::Keyboard::isKeyPressed(sf::Keyboard::X),
				sf::Keyboard::isKeyPressed(sf::Keyboard::Num1),
				sf::Keyboard::isKeyPressed(sf::Keyboard::Num2),
				sf::Keyboard::isKeyPressed(sf::Keyboard::Num3),
				sf::Keyboard::isKeyPressed(sf::Keyboard::Q),
				sf::Keyboard::isKeyPressed(sf::Keyboard::W),
				sf::Keyboard::isKeyPressed(sf::Keyboard::E),
				sf::Keyboard::isKeyPressed(sf::Keyboard::A),
				sf::Keyboard::isKeyPressed(sf::Keyboard::S),
				sf::Keyboard::isKeyPressed(sf::Keyboard::D),
				sf::Keyboard::isKeyPressed(sf::Keyboard::Z),
				sf::Keyboard::isKeyPressed(sf::Keyboard::C),
				sf::Keyboard::isKeyPressed(sf::Keyboard::Num4),
				sf::Keyboard::isKeyPressed(sf::Keyboard::R),
				sf::Keyboard::isKeyPressed(sf::Keyboard::F),
				sf::Keyboard::isKeyPressed(sf::Keyboard::V)
			};

			for (int i = 0; i < 16; ++i) {
				chip8.previousKeypad[i] = chip8.keypad[i];
				chip8.keypad[i] = newKeypad[i];
			}

			chip8.Tick();               // decrement delay amd sound timers
			timerTimer -= timerDelay;
		}

		// Render only when drawFlag is set
		if (chip8.drawFlag) {
			window.clear(sf::Color::Black);
			for (int y = 0; y < Chip8::VIDEO_HEIGHT; ++y) {
				for (int x = 0; x < Chip8::VIDEO_WIDTH; ++x) {
					if (chip8.video[y * Chip8::VIDEO_WIDTH + x]) {
						pixel.setPosition(x * 10, y * 10);
						window.draw(pixel);
					}
				}
			}
			window.display();
			chip8.drawFlag = false;
		}
	}

	return 0;
}
