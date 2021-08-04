# VS Code Config Helper (v3)



## Build instruction

### GCC Debug (not recommended)
For building OpenSSL, export `CONAN_BASH_PATH` to a msys2 bash.exe.

```
mkdir build
cd build
conan install .. -b missing -s compiler=gcc -s compiler.libcxx=libstdc++11 -s compiler.version=11
cmake ..
```

### MSVC Debug

```
mkdir build
cd build
conan install .. -b missing -s build_type=Debug -s compiler.runtime=MTd
cmake ..
```

### MSVC Release

```
mkdir build
cd build
conan install .. -b missing -s build_type=Release -s compiler.runtime=MT
cmake .. --config Release
```