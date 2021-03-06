#include <fstream>
#include <sstream>
#include <unordered_map>

#include <SDL2/SDL.h>

#include "chip8/chip8.hh"

#undef main

std::unordered_map<uint16_t, uint8_t> sdlScanCodeToChip8Key = {
	{SDL_SCANCODE_X, 0x0},
	{SDL_SCANCODE_1, 0x1},
	{SDL_SCANCODE_2, 0x2},
	{SDL_SCANCODE_3, 0x3},
	{SDL_SCANCODE_Q, 0x4},
	{SDL_SCANCODE_W, 0x5},
	{SDL_SCANCODE_E, 0x6},
	{SDL_SCANCODE_A, 0x7},
	{SDL_SCANCODE_S, 0x8},
	{SDL_SCANCODE_D, 0x9},
	{SDL_SCANCODE_Z, 0xA},
	{SDL_SCANCODE_C, 0xB},
	{SDL_SCANCODE_4, 0xC},
	{SDL_SCANCODE_R, 0xD},
	{SDL_SCANCODE_F, 0xE},
	{SDL_SCANCODE_V, 0xF},

};

auto ParseOptions(std::string optionString) {
	std::unordered_map<std::string, std::string> ret;
	while (std::find(optionString.cbegin(), optionString.cend(), '\n') != optionString.cend()) {
		auto key = optionString.substr(0, optionString.find(' '));
		optionString = optionString.substr(optionString.find(' '));
		auto val = optionString.substr(0, optionString.find('\n'));
		optionString = optionString.substr(optionString.find('\n') + 1);
		ret[key] = val;
	}
	return ret;
}

auto main(int argc, char *argv[]) -> int
{
	if (argc != 2) {
		printf("Usage: %s <ROM filename>", argv[0]);
		return 1;
	}

#ifdef _WIN32
	//Necessary(?) to get audio to play on Windows
	//At least on my machine it wouldn't play without this. Could let the user set it before running the program but that doesn't seem very user friendly.
	putenv("SDL_AUDIODRIVER=winmm");
#endif

	SDL_Init(SDL_INIT_AUDIO);

	auto window = SDL_CreateWindow("SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
	auto surface = SDL_GetWindowSurface(window);

	std::unordered_map<std::string, std::string> emuConfig;
	std::ifstream configFile(".emuconfig");
	if (configFile.is_open()) {
		std::stringstream ss;
		ss << configFile.rdbuf();
		emuConfig = ParseOptions(ss.str());
	}

	Chip8 chip8(emuConfig);
	chip8.LoadROM(argv[1]);

	auto framebuffer = chip8.GetFramebuffer();
	auto fbSurface = SDL_CreateRGBSurface(0, 64, 32, 32, 0, 0, 0, 0);
	auto fbSurfacePixels = (uint32_t *)fbSurface->pixels;

	SDL_AudioSpec wanted;
	wanted.callback = chip8.GetAudioCallback();
	wanted.channels = 1;
	wanted.format = AUDIO_F32;
	wanted.freq = 44100;
	wanted.samples = 512;
	wanted.userdata = (void *)&chip8;

	SDL_AudioSpec obtained;

	auto deviceId = SDL_OpenAudioDevice(nullptr, 0, &wanted, &obtained, 0);

	SDL_PauseAudioDevice(deviceId, 0);

	std::array<bool, 16> keys{0};

	SDL_Event e;
	while (true) {
		SDL_PollEvent(&e);
		if (e.type == SDL_QUIT) {
			break;
		}

		if (e.type == SDL_KEYDOWN) {
			if (sdlScanCodeToChip8Key.find(e.key.keysym.scancode) != sdlScanCodeToChip8Key.end()) {
				keys[sdlScanCodeToChip8Key[e.key.keysym.scancode]] = true;
			}
		}
		if (e.type == SDL_KEYUP) {
			if (sdlScanCodeToChip8Key.find(e.key.keysym.scancode) != sdlScanCodeToChip8Key.end()) {
				keys[sdlScanCodeToChip8Key[e.key.keysym.scancode]] = false;
			}
		}

		chip8.SetKeys(keys);

		chip8.Tick();

		framebuffer = chip8.GetFramebuffer();
		for (int y = 0; y < 32; ++y) {
			for (int x = 0; x < 64; ++x) {
				if (framebuffer[y * 64 + x]) {
					fbSurfacePixels[y * 64 + x] = 0xFFFFFFFF;
				} else {
					fbSurfacePixels[y * 64 + x] = 0;
				}
			}
		}

		SDL_BlitScaled(fbSurface, nullptr, surface, nullptr);
		SDL_UpdateWindowSurface(window);
	}

	SDL_FreeSurface(fbSurface);
	SDL_DestroyWindow(window);

	return 0;
}

