#include "Chip8.h"
#include <limits>
#include <fstream>
#include <vector>

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> random(0, 255);

Chip8::Chip8() {
	Reset();
}

void Chip8::Reset() {
	// Program counter starts at 0x200
	pc = 0x200;
	// Inilialize variables
	opcode = 0;
	I = 0;
	sp = 0;
	draw_flag = false;
	std::fill(std::begin(memory), std::end(memory), 0);
	std::fill(std::begin(V), std::end(V), 0);
	std::fill(std::begin(gfx), std::end(gfx), 0);
	std::fill(std::begin(stack), std::end(stack), 0);
	std::fill(std::begin(key), std::end(key), 0);

	// Load font set
	for (int i = 0; i < 80; i++)
	{
		memory[i] = chip8_fontset[i];
	}

	// Reset timers
	delay_timer = 60;
	sound_timer = 60;

	state = State::ON;
}

void Chip8::EmulateCycle(const bool &sound) {
	// Fetch opcode (2 bytes)
	opcode = memory[pc] << 8 | memory[pc + 1];
	pc += 2;
	switch (opcode & 0xF000)
	{
	case 0x0000: // 0x0NNN
		switch (opcode & 0x00FF)
		{
		case 0x00E0: // 0x00E0	disp_clear()
			std::fill(std::begin(gfx), std::end(gfx), 0);
			draw_flag = true;
			break;
		case 0x00EE: // 0x00EE	return;
			pc = stack[--sp] + 2;
			break;
		default: // 0x0NNN
			std::cout << std::hex << "\033[1;31mOpcode (0x0NNN) not implemented, PC: " << pc << ", opcode:\033[0m 0x" << opcode << std::endl;
			break;
		}
		break;
	case 0x1000: // 0x1NNN	goto NNN
		pc = opcode & 0x0FFF;
		break;
	case 0x2000: // 0x2NNN	*(0xNNN)()
		stack[sp++] = pc - 2;
		pc = opcode & 0x0FFF;
		break;
	case 0x3000: // 0x3XNN	if(Vx == NN)
		if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
		{
			pc += 2;
		}
		break;
	case 0x4000: // 0x4YNN	if(Vx != NN)
		if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
		{
			pc += 2;
		}
		break;
	case 0x5000: // 0x5XY0	if(Vx == Vy)
		if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
		{
			pc += 2;
		}
		break;
	case 0x6000: // 0x6XNN	Vx = NN
		V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
		break;
	case 0x7000: // 0x7XNN	Vx += NN
		V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
		break;
	case 0x8000: // 0x8XYN
		switch (opcode & 0x000F)
		{
		case 0x0000: // 0x8XY0	Vx = Vy
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
			break;
		case 0x0001: // 0x8XY1	Vx = Vx | Vy
			V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
			break;
		case 0x0002: // 0x8XY2	Vx = Vx & Vy
			V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
			break;
		case 0x0003: // 0x8XY3	Vx = Vx ^ Vy
			V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
			break;
		case 0x0004: // 0x8XY4	Vx += Vy
			if (V[(opcode & 0x0F00) >> 8] + V[(opcode & 0x00F0) >> 4] > 0xFF)
			{
				V[0xF] = 1;
			}
			else {
				V[0xF] = 0;
			}
			V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
			break;
		case 0x0005: // 0x8XY5	Vx -= Vy
			if (V[(opcode & 0x0F00) >> 8] < V[(opcode & 0x00F0) >> 4])
			{
				V[0xF] = 0;
			}
			else {
				V[0xF] = 1;
			}
			V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
			break;
		case 0x0006: // 0x8XY6	Vx >>= 1
			V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
			V[(opcode & 0x0F00) >> 8] >>= 1;
			break;
		case 0x0007: // 0x8XY7	Vx = Vy - Vx
			if (V[(opcode & 0x00F0) >> 4] < V[(opcode & 0x0F00) >> 8])
			{
				V[0xF] = 0;
			}
			else {
				V[0xF] = 1;
			}
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
			break;
		case 0x000E: // 0x8XYE	Vx <<= 1
			V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
			V[(opcode & 0x0F00) >> 8] <<= 1;
			break;
		default:
			std::cout << std::hex << "\033[1;31mUnknown (0x8XYN) opcode:\033[0m 0x" << opcode << std::endl;
			break;
		}
		break;
	case 0x9000: // 0x9XY0	if(Vx != Vy)
		if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
		{
			pc += 2;
		}
		break;
	case 0xA000: // 0xANNN	I = NNN
		I = opcode & 0x0FFF;
		break;
	case 0xB000: // 0xBNNN	PC = V0 + NNN
		pc = (opcode & 0x0FFF) + V[0];
		break;
	case 0xC000: //0xCXNN	Vx = rand() & NN
		V[(opcode & 0x0F00) >> 8] = random(mt) & (opcode & 0x00FF);
		break;
	case 0xD000: //0xDXYN	draw(Vx,Vy,N)
	{
		unsigned short x = V[(opcode & 0x0F00) >> 8];
		unsigned short y = V[(opcode & 0x00F0) >> 4];
		unsigned short height = opcode & 0x000F;
		unsigned short pixel;

		V[0xF] = 0;
		for (int row = 0; row < height; row++)
		{
			pixel = memory[I + row];
			for (int column = 0; column < 8; column++)
			{
				if ((pixel & (0x80 >> column)) != 0)
				{
					int index = x + column + ((y + row) * 64);
					// Prevent VERS crash and possibly others games
					if (index >= (32 * 64)) {
						while (index >= (32 * 64))
						{
							std::cout << "Index: " << index << std::endl;
							index /= (32 * 64);
						}
					}
					if (gfx[index] == 1)
					{
						V[0xF] = 1;
					}
					
					gfx[index] ^= 1;
				}
			}
		}

		draw_flag = true;
	}
		break;
	case 0xE000: //0xEXNN
		switch (opcode & 0x00FF)
		{
		case 0x009E: //0xEX9E	if(key() == Vx)
			if (key[V[(opcode & 0x0F00) >> 8]] != 0)
			{
				pc += 2;
			}
			break;
		case 0x00A1: //0xEXA1	if(key() != Vx)
			if (key[V[(opcode & 0x0F00) >> 8]] == 0)
			{
				pc += 2;
			}
			break;
		default:
			std::cout << std::hex << "\033[1;31mUnknown (0xE000) opcode:\033[0m 0x" << opcode << std::endl;
			break;
		}
		break;
	case 0xF000: // 0xFXNN
		switch (opcode & 0x00FF)
		{
		case 0x0007: // 0xFX07	Vx = get_delay()
			V[(opcode & 0x0F00) >> 8] = delay_timer;
			break;
		case 0x000A: // 0xFX0A	Vx = get_key()
		{
			bool key_pressed = false;

			for (int i = 0; i < 16; i++)
			{
				if (key[i] != 0) {
					V[(opcode & 0x0F00) >> 8] = i;
					key_pressed = true;
				}
			}
			if (!key_pressed) {
				pc -= 2;
			}
		}
			break;
		case 0x0015: // 0xFX15	delay_timer(Vx)
			delay_timer = V[(opcode & 0x0F00) >> 8];
			break;
		case 0x0018: // 0xFX18	sound_timer(Vx)
			sound_timer = V[(opcode & 0x0F00) >> 8];
			break;
		case 0x001E: // 0xFX1E	I += Vx
			// For Commodore Amiga carry flag is set
			if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF)
			{
				V[0xF] = 1;
			}
			else {
				V[0xF] = 0;
			}
			I += V[(opcode & 0x0F00) >> 8];
			break;
		case 0x0029: // 0xFX29	I = sprite_addr[Vx]
			I = V[(opcode & 0x0F00) >> 8] * 0x5;
			break;
		case 0x0033: // 0xFX33	set_BCD(Vx);	*(I+0)=BCD(3);	*(I+1)=BCD(2);	*(I+2)=BCD(1);
			memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
			memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
			memory[I + 2] = V[(opcode & 0x0F00) >> 8] % 10;
			break;
		case 0x0055: // 0xFX55	reg_dump(Vx, &I)
			for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
			{
				memory[I + i] = V[i];
			}
			// I += ((opcode & 0x0F00) >> 8) + 1; Original chip8 interpreter
			break;
		case 0x0065: // 0xFX65	reg_load(Vx, &I)
			for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
			{
				V[i] = memory[I + i];
			}
			// I += ((opcode & 0x0F00) >> 8) + 1; Original chip8 interpreter
			break;
		default:
			std::cout << std::hex << "\033[1;31mUnknown (0xFXNN) opcode \033[0m: 0x" << opcode << std::endl;
			break;
		}
		break;
	default:
		std::cout << std::hex << "\033[1;31mUnknown opcode \033[0m: 0x" << opcode << std::endl;
		break;
	}

	//
	if (delay_timer > 0) {
		delay_timer--;
	}
	if (sound_timer > 0) {
		if (sound_timer == 1 && sound) {
			std::cout << "\a" << std::flush;
		}
		sound_timer--;
	}
}

bool Chip8::LoadGame(const std::string& dir) {
	std::ifstream game(dir, std::ios::binary);

	if (game.is_open()) {
		// Copies all data into buffer
		std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(game), {});
		std::cout << "Loaded game: " << dir << std::endl;
		std::cout << "Bytes: " << buffer.size() << std::endl;

		for (int i = 0; i < buffer.size(); i++)
		{
			memory[i + 512] = buffer[i];
		}

		return true;
	}
	else {
		std::cout << "Failed to open game: " << dir << std::endl;
		return false;
	}
}

bool Chip8::GetDrawFlag() {
	return draw_flag;
}

void Chip8::SetDrawFlag(const bool& draw_flag) {
	this->draw_flag = draw_flag;
}

unsigned char Chip8::GetPixel(const int &position) {
	return gfx[position];
}

State Chip8::GetState() {
	return state;
}

void Chip8::SetState(const State& state) {
	this->state = state;
}

void Chip8::SetKey(const int &index, const bool &pressed) {
	key[index] = pressed;
}