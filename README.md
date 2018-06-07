This repository contains an implementation of a VM for the [CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) system. The emulator is built in C++ using SDL2 for cross-platform graphics and audio.

## Building
[Premake 5](https://premake.github.io/download.html) is used for building. The Premake file assumes dependencies are placed in a file structure similar to the one created by [vcpkg](https://github.com/Microsoft/vcpkg). If using vcpkg, you can copy the "installed" folder created by vcpkg into the root directory of the repository and things should just work.

## TODO
- Fix implementation of random
- Implement audio
- Add a way to configure various things (emulated clock frequency, delay/sound timer frequency, window size, keybinds, etc.)
- Clean up main tick function
