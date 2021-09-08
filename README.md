# VS Code Config Helper (v3)

[![](https://img.shields.io/github/workflow/status/Guyutongxue/VSCodeConfigHelper3/CMake)](https://github.com/Guyutongxue/VSCodeConfigHelper3/actions/workflows/cmake.yml)

## Support Platforms

- GUI & CLI: Windows 10 and above
- CLI:
    - Ubuntu or other Debian-based Linux distro
    - Intel Mac

## Usage

## GUI

Follow the instruction on GUI, see also [Demo](https://b23.tv/av292212272).

## CLI

Example:

```sh
# Configure <Workspace Folder Path>, with external terminal; Open vscode after config.
./vscch -eoy -w <Workspace Folder Path>
```

Use `./vscch -h` to show complete options list.

## Build instruction

### MinGW Debug (not recommended)
For building OpenSSL, export `CONAN_BASH_PATH` to a msys2 bash.exe.

```
mkdir build
cd build
conan install .. -b missing -s compiler=gcc -s compiler.libcxx=libstdc++11 -s compiler.version=11
cmake ..
mingw32-make
```

### MSVC Debug

```
mkdir build
cd build
conan install .. -b missing -s build_type=Debug -s compiler.runtime=MTd
cmake ..
cmake --build .
```

### MSVC Release

```
mkdir build
cd build
conan install .. -b missing -s build_type=Release -s compiler.runtime=MT
cmake ..
cmake --build . --config MinSizeRel
```
## GCC (Linux)

```
mkdir build
cd build
conan install .. -b missing -s compiler.libcxx=libstdc++11
cmake ..
make
```

## Apple Clang

```
mkdir build
cd build
conan install .. -b missing
cmake ..
cmake --build .
```