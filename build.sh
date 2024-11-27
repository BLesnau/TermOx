#!/bin/bash

# Clean the build directory if necessary
if [ -d "build" ]; then
  echo "Cleaning previous build..."
  rm -rf build
fi

# Create and navigate to the build directory
mkdir build
cd build

# Run the CMake command
cmake -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
      -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
      -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
      -DCMAKE_CXX_STANDARD=20 \
      -DICU_ROOT=/opt/homebrew/opt/icu4c \
      -DICU_INCLUDE_DIR=/opt/homebrew/opt/icu4c/include \
      -DICU_LIBRARIES="/opt/homebrew/opt/icu4c/lib/libicuuc.dylib;/opt/homebrew/opt/icu4c/lib/libicui18n.dylib;/opt/homebrew/opt/icu4c/lib/libicudata.dylib" \
      ..
 make Term0x