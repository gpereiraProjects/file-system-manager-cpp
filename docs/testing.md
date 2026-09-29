# Testing strategy

## Approach

The project uses a lightweight C++ characterization suite integrated with
CTest. Tests exercise the public behavior against isolated temporary directory
fixtures rather than relying on the developer's personal files.

Characterization tests were chosen to protect existing behavior while the
original academic implementation was modernized in small, verifiable steps.
They now serve as regression tests for the portfolio version.

## Test map

The 30 scenarios cover:

| Area | Verified behavior |
| --- | --- |
| Loading and statistics | Tree loading, counts, byte totals, largest items, invalid roots |
| Public API | Unloaded state, `const` queries, wide sizes, typed optional results |
| Paths | Portable normalization, exact search, exact file and directory moves |
| Console boundary | Invalid input, EOF, rename presentation, silent domain operations |
| Tree safety | Ownership transfers, cyclic-move rejection, symbolic-link cycles |
| Mutations | Removal statistics, recursive removal, root guard, copy collisions |
| Renaming | Disk/model consistency, extension metadata, validation, collisions, rollback |
| XML | Round trip, escaping, structural variants, malformed input, atomic replacement |

## Running the suite

```console
cmake --preset debug
cmake --build --preset debug
ctest --test-dir build/debug --output-on-failure
```

The presets treat compiler warnings as errors. A successful local run therefore
checks both the tests and the configured compiler warning policy.

The same sequence is available through the local verification scripts:

```powershell
.\scripts\verify.ps1
```

```console
./scripts/verify.sh
```

With no argument, the scripts validate Debug followed by Release. Pass one of
those preset names to run only that configuration.

## Isolation

Each file-system scenario creates a unique fixture under the operating
system's temporary directory. Fixture destruction removes that directory after
the test. The program never uses a real user directory as test input.

The logger also writes to the temporary directory during tests, keeping runtime
artifacts out of the repository.

## Platform note

Creating symbolic links on Windows may require Developer Mode or an elevated
token. If the operating system denies link creation, the test reports that the
platform capability is unavailable; on supported configurations it verifies
that loading ignores symbolic links and cannot recurse through a cycle.

## Continuous integration

The workflow in `.github/workflows/ci.yml` configures, builds, and tests both
the Debug and Release presets on:

- Windows with MinGW-w64 GCC;
- Linux with GCC;
- macOS with Clang.

This matrix detects compiler assumptions, path-handling differences, and
platform-specific regressions before changes are accepted. The workflow can
also be started manually from GitHub and uses read-only repository permissions.
