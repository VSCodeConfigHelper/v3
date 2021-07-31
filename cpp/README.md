


## MSVC Debug

```
mkdir build
cd build
conan install .. -b missing -s build_type=Debug -s compiler.runtime=MTd
cmake ..
```

## MSVC Release

```
mkdir build
cd build
conan install .. -b missing -s build_type=Release -s compiler.runtime=MT
cmake .. --config Release
```