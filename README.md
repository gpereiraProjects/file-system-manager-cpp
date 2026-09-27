# File System Manager

An object-oriented C++17 console application that loads a directory tree into
memory and provides operations for inspecting, searching, moving, copying,
renaming, exporting, and importing file-system data.

This repository is being prepared as a portfolio version of an academic
Object-Oriented Programming project. The implementation currently targets
Windows because it uses the Windows console API.

## Requirements

- CMake 3.22 or newer
- A C++17 compiler, such as Visual Studio 2022 or MinGW-w64
- Ninja when using the included CMake presets

The project is currently developed with the MSYS2 UCRT64 toolchain. Opening an
MSYS2 UCRT64 terminal makes CMake, Ninja, and GCC available automatically.

## Build

From a terminal opened in the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The executable is generated under `build/bin`.

Alternatively, use one of the included presets:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
```

Replace `windows-debug` with `windows-release` for an optimized build. Preset
executables are generated under `build/<preset>/bin`. Both presets treat
compiler warnings as errors so regressions are detected during development.

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
|-- tests/       Automated characterization tests
|-- main.cpp     Application entry point
`-- CMakeLists.txt
```

## Tests

Build the selected preset and run its test suite with CTest:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --test-dir build/windows-debug --output-on-failure
```

The current characterization suite protects directory loading, statistics,
search, largest-file selection, tree output, invalid-path handling, and XML
export/import behavior while the implementation is refactored. It also covers
file and directory ownership transfers after move operations.

## Memory ownership

The in-memory tree uses `std::unique_ptr` to express exclusive ownership:

- `SistemaFicheiros` owns the root directory;
- each `Diretoria` owns its files and child directories;
- move operations transfer ownership instead of copying or reusing owning raw
  pointers;
- XML streams and temporary trees rely on automatic lifetime management.

The original assignment's public `std::string*` return types remain available
for compatibility. Internally, recursive searches use values and
`std::optional`, and callers immediately wrap compatibility results in
`std::unique_ptr`.

The architecture overview and usage examples will be expanded as the
portfolio refactoring progresses.
