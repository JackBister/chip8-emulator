#include "chip8/chip8.hh"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
//#include <filesystem>
#include <fstream>
#include <random>
#include <unordered_map>

//Template thing I've been experimenting with to give a nice syntax for the &ing and >>ing you end up doing with opcodes. May come in handy in the future:
auto BreakNibbles(uint16_t const val, uint8_t& nib) -> void
{
	nib = val & 0xF;
}

template<typename ... outs>
auto BreakNibbles(uint16_t val, uint8_t& nib, outs& ... nibs) -> void
{
	nib = val & 0xF;
	BreakNibbles(val >> 4, nibs...);
}

uint8_t chip8_fontset[80] =
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

struct Chip8Config
{
	int freq = 0;
	float volume = 0.0f;

	int delay_freq = 60;
	int sound_freq = 60;
};

struct Chip8::Pimpl
{
	static auto AudioCallback(void * userdata, uint8_t * buf, int len) -> void;

	auto Op_Draw(uint8_t VX, uint8_t VY, uint8_t N) -> void;
	auto Tick() -> void;

	std::array<uint8_t, 64 * 32> framebuffer{0};
	std::array<bool, 16> keys{0};
	std::array<uint8_t, 4096> memory{0};
	std::array<uint8_t, 16> registers{0};
	std::array<uint16_t, 16> stack{0};

	uint16_t i = 0;
	uint16_t pc = 0x200;
	uint16_t sp = 0;

	uint8_t delay_timer = 0;
	std::atomic_uint8_t sound_timer = 0;

	Chip8Config config;
	std::chrono::high_resolution_clock::time_point lastRefresh;
};

auto Chip8::Pimpl::AudioCallback(void * userdata, uint8_t * buf, int len) -> void
{
	auto impl = (Chip8 *)userdata;
	auto realBuf = (float *)buf;

	if (impl->pimpl->sound_timer > 0) {
		for (int i = 0; i < len / sizeof(float); ++i) {
			realBuf[i] = impl->pimpl->config.volume;
		}
	} else {
		for (int i = 0; i < len / sizeof(float); ++i) {
			realBuf[i] = 0.f;
		}
	}
}

auto Chip8::Pimpl::Op_Draw(uint8_t X, uint8_t Y, uint8_t N) -> void
{
	registers[0xF] = 0;
	for (int y = 0; y < N; ++y) {
		//line is an 8 pixel line of the sprite
		auto const line = memory[i + y];
		for (int x = 0; x < 8; ++x) {
			auto finalX = (X + x) % 64;
			auto finalY = (Y + y) % 32;
			//pixel is the xth pixel of the line from the left
			auto const pixel = line & (0x80 >> x);
			if (pixel != 0) {
				auto const dstPixel = &framebuffer[finalX + finalY * 64];
				if (*dstPixel == 1) {
					registers[0xF] = 1;
				}
				*dstPixel ^= 1;
			}
		}
	}
}

auto Chip8::Pimpl::Tick() -> void
{
	uint16_t const opcode = memory[pc] << 8
		| memory[pc + 1];

	auto timeSinceRefresh = std::chrono::high_resolution_clock::now() - lastRefresh;

	auto shouldIncrement = true;

	switch ((opcode & 0xF000) >> 12) {
	case 0: {
		if (opcode == 0x00E0) {
			framebuffer.fill(0);
			break;
		}
		if (opcode == 0x00EE) {
			sp--;
			pc = stack[sp];
			break;
		}
		printf("[STUB] Chip8::Tick(%d) RCA1802 call", __LINE__);
		break;
	}
	case 1: {
		pc = (opcode & 0xFFF);
		shouldIncrement = false;
		break;
	}
	case 2: {
		stack[sp] = pc;
		sp++;
		pc = (opcode & 0xFFF);
		shouldIncrement = false;
		break;
	}
	case 3: {
		uint8_t const reg = (opcode & 0xF00) >> 8;
		uint8_t const val = (opcode & 0xFF);
		if (registers[reg] == val) {
			pc += 2;
		}
		break;
	}
	case 4: {
		uint8_t const reg = (opcode & 0xF00) >> 8;
		uint8_t const val = (opcode & 0xFF);
		if (registers[reg] != val) {
			pc += 2;
		}
		break;
	}
	case 5: {
		uint8_t const reg_a = (opcode & 0xF00) >> 8;
		uint8_t const reg_b = (opcode & 0xF0) >> 4;
		if (registers[reg_a] == registers[reg_b]) {
			pc += 2;
		}
		break;
	}
	case 6: {
		uint8_t const reg = (opcode & 0xF00) >> 8;
		uint8_t const val = opcode & 0xFF;
		registers[reg] = val;
		break;
	}
	case 7: {
		uint8_t const reg = (opcode & 0xF00) >> 8;
		uint8_t const val = opcode & 0xFF;
		registers[reg] += val;
		break;
	}
	case 8: {
		uint8_t	const reg_a = (opcode & 0xF00) >> 8;
		uint8_t const reg_b = (opcode & 0xF0) >> 4;
		uint8_t const op = (opcode & 0xF);
		switch (op) {
		case 0: {
			registers[reg_a] = registers[reg_b];
			break;
		}
		case 1: {
			registers[reg_a] |= registers[reg_b];
			break;
		}
		case 2: {
			registers[reg_a] &= registers[reg_b];
			break;
		}
		case 3: {
			registers[reg_a] ^= registers[reg_b];
			break;
		}
		case 4: {
			if (registers[reg_b] > 0xFF - registers[reg_a]) {
				registers[0xF] = 1;
			} else {
				registers[0xF] = 0;
			}
			registers[reg_a] += registers[reg_b];
			break;
		}
		case 5: {
			if (registers[reg_b] > registers[reg_a]) {
				registers[0xF] = 1;
			} else {
				registers[0xF] = 0;
			}
			registers[reg_a] -= registers[reg_b];
			break;
		}
		case 6: {
			registers[0xF] = registers[reg_b] & 1;
			registers[reg_a] = registers[reg_b] >> 1;
			break;
		}
		case 7: {
			if (registers[reg_a] > registers[reg_b]) {
				registers[0xF] = 1;
			} else {
				registers[0xF] = 0;
			}
			registers[reg_a] = registers[reg_b] - registers[reg_a];
			break;
		}
		case 0xE: {
			registers[0xF] = registers[reg_b] & 0x80;
			registers[reg_b] <<= 1;
			registers[reg_a] = registers[reg_b];
			break;
		}
		default: {
			printf("[ERROR] Chip8::Tick(%d) default in inner switch\n", __LINE__);
			break;
		}
		}
		break;
	}
	case 9: {
		uint8_t	const reg_a = (opcode & 0xF00) >> 8;
		uint8_t const reg_b = (opcode & 0xF0) >> 4;
		if (registers[reg_a] != registers[reg_b]) {
			pc += 2;
		}
		break;
	}
	case 0xA: {
		i = (opcode & 0xFFF);
		break;
	}
	case 0xB: {
		pc = registers[0] + (opcode & 0xFFF);
		shouldIncrement = false;
		break;
	}
	case 0xC: {
		uint8_t const reg = (opcode & 0xF00) >> 8;
		uint8_t const val = (opcode & 0xFF);
		uint8_t const randVal = rand();
		registers[reg] = randVal & val;
	}
	case 0xD: {
		uint8_t const X = registers[(opcode & 0x0F00) >> 8];
		uint8_t const Y = registers[(opcode & 0x00F0) >> 4];
		uint8_t const N = opcode & 0x000F;
		Op_Draw(X, Y, N);
		break;
	}
	case 0xE: {
		auto op = opcode & 0xFF;
		auto reg = (opcode & 0x0F00) >> 8;
		auto key = registers[reg];
		switch (op) {
		case 0x9E: {
			if (keys[key]) {
				pc += 2;
			}
			break;
		}
		case 0xA1: {
			if (!keys[key]) {
				pc += 2;
			}
			break;
		}
		default: {
			printf("Chip8::Tick(%d) default in inner switch", __LINE__);
			break;
		}
		}
		break;
	}
	case 0xF: {
		uint8_t const reg = (opcode & 0xF00) >> 8;
		uint8_t const op = (opcode & 0xFF);
		if (op == 0x1E) {
			i += registers[reg];
			break;
		} else if (op == 0x07) {
			registers[reg] = delay_timer;
			break;
		} else if (op == 0x0A) {
			printf("[STUB] 0xFX0A\n");
			break;
		} else if (op == 0x15) {
			delay_timer = registers[reg];
			break;
		} else if (op == 0x18) {
			sound_timer = registers[reg];
			break;
		} else if (op == 0x29) {
			i = registers[reg] * 5 + 0x50;
			break;
		} else if (op == 0x33) {
			memory[i] = registers[reg] / 100;
			memory[i + 1] = (registers[reg] / 10) % 10;
			memory[i + 2] = (registers[reg] % 100) % 10;
			break;
		} else if (op == 0x55) {
			for (uint8_t j = 0; j <= reg; ++j) {
				memory[i] = registers[j];
				i++;
			}
			break;
		} else if (op == 0x65) {
			for (uint8_t j = 0; j <= reg; ++j) {
				registers[j] = memory[i];
				i++;
			}
			break;
		}
	}
	default: {
		printf("[ERROR] Chip8::Tick(%d) default in switch.", __LINE__);
		break;
	}
	}

	if (shouldIncrement) {
		pc += 2;
	}

	if (timeSinceRefresh > std::chrono::nanoseconds(1000000000) / config.delay_freq) {
		if (delay_timer > 0) {
			delay_timer--;
		}
	}
	if (timeSinceRefresh > std::chrono::nanoseconds(1000000000) / config.sound_freq) {
		if (sound_timer > 0) {
			sound_timer--;
		}
		lastRefresh = std::chrono::high_resolution_clock::now();
	}
}

Chip8::Chip8(std::unordered_map<std::string, std::string>& config) : pimpl(new Chip8::Pimpl())
{
	memcpy(&pimpl->memory[0x50], chip8_fontset, sizeof(chip8_fontset));
	pimpl->lastRefresh = std::chrono::high_resolution_clock::now();

	if (config.find("freq") != config.end()) {
		pimpl->config.freq = atoi(config["freq"].c_str());
		if (pimpl->config.freq < 1) {
			pimpl->config.freq = 0;
		}
	}
	if (config.find("volume") != config.end()) {
		pimpl->config.volume = atof(config["volume"].c_str());
	}
	if (config.find("delay_freq") != config.end()) {
		pimpl->config.delay_freq = atof(config["delay_freq"].c_str());
		if (pimpl->config.delay_freq < 1) {
			pimpl->config.delay_freq = 60;
		}
	}
	if (config.find("sound_freq") != config.end()) {
		pimpl->config.sound_freq = atof(config["sound_freq"].c_str());
		if (pimpl->config.sound_freq < 1) {
			pimpl->config.sound_freq = 60;
		}
	}
	if (config.find("seed") != config.end()) {
		srand(atof(config["seed"].c_str()));
	} else {
		srand(time(nullptr));
	}
}

Chip8::~Chip8() = default;

auto Chip8::GetAudioCallback() -> SDL_AudioCallback
{
	return pimpl->AudioCallback;
}

auto Chip8::GetFramebuffer() -> std::array<uint8_t, 64 * 32>&
{
	return pimpl->framebuffer;
}

auto Chip8::LoadROM(std::string const& fileName) -> void
{
	std::ifstream file(fileName, std::ios_base::binary | std::ios_base::in);
	file.read((char *)&pimpl->memory[0x200], 0x7FF);
}

auto Chip8::SetKeys(std::array<bool, 16> keys) -> void
{
	pimpl->keys = keys;
}

auto Chip8::Tick() -> void
{
	pimpl->Tick();
}
