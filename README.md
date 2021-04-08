# sjasmplus
SJAsmPlus Z80 Assembler

# Building from source
## Requirements
- [CMake](https://cmake.org/)
- [Boost](https://www.boost.org/) C++ libraries
```
mkdir build
cd build
cmake ..
make
```
 To build on non-Unix systems (Windows / Visual Studio) see [CMake documentation on generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html).
 The necessary steps are something like the following:
```
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
# The above will generate a solution file for Visual Studio.
# Now open the generated solution file in Visual studio and build the project.
```


# Usage

See [Documentation](https://github.com/sjasmplus/sjasmplus/wiki) (Wiki)
