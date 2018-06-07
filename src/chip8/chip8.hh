#pragma once

#include <array>
#include <memory>
#include <string>

class Chip8
{
public:
	Chip8();
	~Chip8();

	auto GetFramebuffer() -> std::array<uint8_t, 64 * 32>&;
	auto LoadROM(std::string const& fileName) -> void;
	auto SetKeys(std::array<bool, 16>) -> void;
	auto Tick() -> void;

private:
	struct Pimpl;

	std::unique_ptr<Pimpl> const pimpl;
};
