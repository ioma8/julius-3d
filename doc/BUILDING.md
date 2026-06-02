# Building Julius

To build Julius, you need:

- a compiler
- `cmake`
- `SDL2`
- `SDL2_mixer`
- `libpng` (optional, a bundled copy will be used if not found)

After downloading or cloning the sources, run the following commands:

	$ mkdir build && cd build
	$ cmake ..
	$ make

This results in a `julius` executable for your platform.

See [Running Julius (wiki)](https://github.com/bvschaik/julius/wiki/Running-Julius) for instructions on how to configure Julius.

See [Building Julius (Wiki)](https://github.com/bvschaik/julius/wiki/Building-Julius) for detailed build instructions and additional CMake flags.

## macOS

Install Xcode Command Line Tools and the required Homebrew packages:

	$ xcode-select --install
	$ brew install cmake sdl2 sdl2_mixer libpng

Then configure and build:

	$ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	$ cmake --build build --parallel

This creates `build/julius.app`.

To make the app bundle self-contained, copy its Homebrew libraries into the
bundle, and apply an ad-hoc signature, run:

	$ cmake --install build

You can then run:

	$ open build/julius.app

Julius still needs the original Caesar 3 game data at runtime. If the app cannot
find files such as `c3.eng` or `c3_mm.eng`, point it at your Caesar 3
installation when prompted, or pass the data directory as the final argument:

	$ build/julius.app/Contents/MacOS/julius /path/to/Caesar3
