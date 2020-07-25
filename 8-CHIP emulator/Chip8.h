#pragma once
#include <string>
#include <iostream>
#include <random>

enum class State
{
	ON,
	OFF
};

const unsigned char chip8_fontset[80] =
{
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

class Chip8 {
private:
	unsigned short opcode;
	// Memory (4K)
	unsigned char memory[4096];
	// Registers uint8_t
	uint8_t V[16];
	// Register index
	unsigned short I;
	// Program counter
	unsigned short pc;
	// Pixel state
	unsigned char gfx[64 * 32];
	// Timers
	unsigned char delay_timer;
	unsigned char sound_timer;
	// Stack for storing the program counter
	unsigned short stack[16];
	// Stack counter
	unsigned short sp;
	// Keyboard
	unsigned char key[16];
	// Draw flag
	bool draw_flag;
	// State of the emulator
	State state;
public:
	Chip8();
	void Reset();
	bool LoadGame(const std::string& dir);
	void EmulateCycle(const bool &sound_timer);

	bool GetDrawFlag();
	State GetState();
	unsigned char GetPixel(const int &position);
	void SetDrawFlag(const bool& draw_flag);
	void SetState(const State &state);
	void SetKey(const int &index, const bool &pressed);
};