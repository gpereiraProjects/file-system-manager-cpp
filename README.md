# File System Manager

An object-oriented C++17 console application that loads a directory tree into
memory and provides operations for inspecting, searching, moving, copying,
renaming, exporting, and importing file-system data.

This repository is being prepared as a portfolio version of an academic
Object-Oriented Programming project. The implementation currently targets
Windows because it uses the Windows console API.

## Requirements

- CMake 3.20 or newer
- A C++17 compiler, such as Visual Studio 2022 or MinGW-w64

## Build

From a terminal opened in the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The executable is generated under `build/bin`.

To enable warnings as errors:

```powershell
cmake -S . -B build -DFILE_SYSTEM_MANAGER_WARNINGS_AS_ERRORS=ON
cmake --build build --config Release
```

## Project structure

```text
.
|-- include/     Public class declarations
|-- src/         Class implementations
|-- main.cpp     Application entry point
`-- CMakeLists.txt
```

The documentation, automated tests, architecture overview, and usage examples
will be expanded as the portfolio refactoring progresses.
