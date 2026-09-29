# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the project follows [Semantic Versioning](https://semver.org/).

## [1.0.0] - Unreleased

### Added

- Cross-platform CMake build with Debug and Release presets.
- Automated characterization suite with 30 scenarios.
- Continuous integration for Windows, Linux, and macOS.
- Local pre-publication verification scripts.
- XML import and export with structural and semantic validation.
- Architecture, testing, build, and usage documentation.

### Changed

- Replaced owning raw pointers with `std::unique_ptr` ownership.
- Modernized the public API with values, `std::optional`, and typed enums.
- Made size calculations wide and read-only queries `const`.
- Separated console presentation from the domain model.
- Made path handling portable and exact across supported platforms.

### Fixed

- Strict Clang warnings detected by the macOS continuous-integration build.
- Unbounded Windows test execution and symbolic-link fixture cleanup.
- Deprecated Node.js runtime warnings from the checkout workflow action.
- Invalid menu input and end-of-file handling.
- File and directory rename validation, collisions, and rollback.
- Ambiguous searches and moves when names are repeated.
- Directory traversal through symbolic-link cycles.
- Partial state changes after failed file-system or XML operations.
