# File System Manager

An object-oriented C++17 console application that loads a directory tree into
memory and provides operations for inspecting, searching, moving, copying,
renaming, exporting, and importing file-system data.

This repository is being prepared as a portfolio version of an academic
Object-Oriented Programming project. The implementation supports Windows,
Linux, and macOS through the C++17 standard library and a small isolated
Windows console adaptation.

## Requirements

- CMake 3.22 or newer
- A C++17 compiler, such as MSVC, GCC, or Clang
- Ninja when using the included CMake presets

The project is currently developed with the MSYS2 UCRT64 toolchain on Windows,
but the build does not depend on MSYS2 or Windows-specific project files.

## Build

From a terminal opened in the repository root:

```console
cmake -S . -B build
cmake --build build --config Release
```

The executable is generated under `build/bin`.

Alternatively, use one of the included presets:

```console
cmake --preset debug
cmake --build --preset debug
```

Replace `debug` with `release` for an optimized build. Preset
executables are generated under `build/<preset>/bin`. Both presets treat
compiler warnings as errors so regressions are detected during development.

To enable warnings as errors:

```console
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

```console
cmake --preset debug
cmake --build --preset debug
ctest --test-dir build/debug --output-on-failure
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

The public API returns values and `std::optional` results, so callers never
receive owning raw pointers or need to call `delete`. Item categories use the
strong `SistemaFicheiros::TipoItem` enumeration instead of numeric or textual
sentinels, and collection queries return their results by value.

## Object-oriented design

The domain model is intentionally separated from the console interface:

- `Item` is an abstract base class with a virtual destructor and a polymorphic
  file-type query;
- `Ficheiro` and `Diretoria` provide the concrete file and directory behavior;
- a directory exposes its owned collection as a read-only view, while
  controlled methods perform additions and ownership transfers;
- public headers use qualified standard-library names and do not leak namespace
  directives into consumers;
- count queries use `std::size_t`, byte totals use `std::uintmax_t`, and
  read-only operations are callable through `const SistemaFicheiros&`;
- domain operations never write to the console: tree rendering returns text,
  renaming returns a typed result, and the menu translates results into
  user-facing messages;
- the reusable core library contains the domain and persistence code, while
  `Menu.cpp` is compiled only into the console executable.

## Operational safety

Operations that affect the physical file system are exercised only inside
temporary test fixtures. The implementation maintains these invariants:

- failed directory loads and XML imports preserve the previously loaded tree;
- directory loading ignores symbolic links and records canonical directory
  identities, preventing cycles and repeated traversal through aliases;
- direct searches, metadata queries and move operations resolve absolute paths
  or paths relative to the loaded root, so repeated names cannot select an
  arbitrary item; `.` identifies the root directory;
- file and directory moves reject duplicates and cyclic directory moves;
- failed physical moves attempt to roll back before returning an error;
- removals update the disk before committing the in-memory change;
- batch copies add an in-memory item only after its physical copy succeeds;
- batch renames validate portable leaf names and every destination before any
  change, then roll back completed disk renames if a later one fails;
- renaming a file also keeps its extension metadata synchronized;
- directory sizes and total file bytes are recalculated after mutations.

## XML persistence

XML snapshots are written to a temporary file and only replace the destination
after the complete document has been flushed successfully. If replacement
fails, the previous snapshot is restored whenever one existed.

The XML reader builds a temporary tree and commits it only after validating the
entire document. It supports escaped and numeric character references, flexible
attribute order and self-closing file elements. It rejects malformed nesting,
unknown or duplicate attributes, invalid and overflowing sizes, inconsistent
directory totals, duplicate child names, multiple roots, unknown entities and
non-portable names, including reserved Windows device names and names that
could escape their parent path. Child-name collisions are checked without
ASCII case sensitivity so a snapshot remains portable across supported
platforms. Unsupported XML constructs are rejected rather than interpreted.

## Portability

Platform-independent paths are composed and normalized with
`std::filesystem`. Console clearing uses ANSI terminal sequences, while the
Windows-only UTF-8 and virtual-terminal setup is isolated inside `Utils.cpp`.
Date conversion selects the thread-safe API provided by each operating system,
and CMake links the platform's standard thread implementation through
`Threads::Threads`.

The continuous-integration workflow builds and tests the same source with
MSVC on Windows, GCC on Linux, and Clang on macOS.
