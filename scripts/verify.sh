#!/usr/bin/env sh

set -eu

selection="${1:-all}"

case "$selection" in
  all|debug|release)
    ;;
  *)
    echo "Usage: $0 [all|debug|release]" >&2
    exit 2
    ;;
esac

if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake was not found in PATH. Install CMake 3.22 or newer." >&2
  exit 1
fi

if ! command -v ctest >/dev/null 2>&1; then
  echo "CTest was not found in PATH. Install the CMake test tools." >&2
  exit 1
fi

if [ "$selection" = "all" ]; then
  set -- debug release
else
  set -- "$selection"
fi

for preset do
  echo "Configuring the $preset preset..."
  cmake --preset "$preset"

  echo "Building the $preset preset..."
  cmake --build --preset "$preset"

  echo "Testing the $preset preset..."
  ctest --preset "$preset"
done

echo "Verification completed successfully."
