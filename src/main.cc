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

auto main(int argc, char *argv[]) -> int
{
	if (argc != 2) {
		printf("Usage: %s <ROM filename>", argv[0]);
		return 1;
	}

	auto window = SDL_CreateWindow("SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
	auto surface = SDL_GetWindowSurface(window);

	Chip8 chip8;
	chip8.LoadROM(argv[1]);


	auto framebuffer = chip8.GetFramebuffer();
	auto fbSurface = SDL_CreateRGBSurface(0, 64, 32, 32, 0, 0, 0, 0);
	auto fbSurfacePixels = (uint32_t *)fbSurface->pixels;

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

