#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>

//TODO:
#include <SDL2/SDL.h>


class Chip8
{
public:
	Chip8(std::unordered_map<std::string, std::string>&);
	~Chip8();

	auto GetAudioCallback() -> SDL_AudioCallback;
	auto GetFramebuffer() -> std::array<uint8_t, 64 * 32>&;
	auto LoadROM(std::string const& fileName) -> void;
	auto SetKeys(std::array<bool, 16>) -> void;
	auto Tick() -> void;

private:
	struct Pimpl;

	std::unique_ptr<Pimpl> const pimpl;
};
