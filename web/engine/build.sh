#!/usr/bin/env bash
# Builds web/thudi.wasm from the plugin's DSP sources with wasi-sdk (WASI_SDK=/path/to/wasi-sdk).
set -euo pipefail
cd "$(dirname "$0")/../.."
CXX="${WASI_SDK:?set WASI_SDK}/bin/clang++"
"$CXX" --target=wasm32-wasip1 -mexec-model=reactor -O3 -std=c++17 -fno-exceptions -ffast-math \
  -msimd128 -mbulk-memory -DNDEBUG -D_LIBCPP_HAS_NO_THREADS \
  -Isrc/dsp web/engine/thudi_wasm.cpp src/dsp/Voice.cpp src/dsp/PadChain.cpp src/dsp/Engine.cpp src/dsp/SampleData.cpp \
  -Wl,--strip-all -Wl,--initial-memory=33554432 -Wl,--max-memory=268435456 \
  -o web/app/thudi.wasm
ls -l web/app/thudi.wasm
