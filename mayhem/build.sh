#!/usr/bin/env bash
#
# mayhem/build.sh — build pt-pc's parser fuzz harnesses and its standalone unit tests.
#
# Upstream's full CMake build needs Vulkan/SDL3/whisper/imgui/... and a GPU-capable toolchain, none of
# which the parsers need. So, like upstream's own pt_engine library, we compile the engine's
# self-contained parser sources (src/engine/**.cpp: JSON, mods, fpk, fox2, Wwise banks, animation,
# UI, VFX, update check, mod Lua host, sound packages, wem/Wwise Vorbis, qar/ftex) into a static library — once with the fuzz flags for the
# harnesses, once with normal flags for upstream's tests (tests/mods_test.cpp, tests/test_strcode.cpp).
# Dependencies (glm, Lua 5.1.5, Vulkan headers, ogg/vorbis) come from /opt/toolchains/deps, baked by mayhem/Dockerfile.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
: "${SRC:=/mayhem}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

GLM=/opt/toolchains/deps/glm-1.0.3
LUA=/opt/toolchains/deps/lua-5.1.5
# Vulkan headers only (engine/render/mesh.h includes render/vk.h); nothing links against Vulkan.
VOLK=/opt/toolchains/deps/volk-1.4.350
VMA=/opt/toolchains/deps/vma-3.4.0
VKH=/opt/toolchains/deps/vulkan-headers-1.4.350
# ogg+vorbis decoder and the Wwise codebooks as one static lib (upstream's pt_vorbis), for audio/wwise_vorbis.cpp.
VORBIS=/opt/toolchains/deps/vorbis-1.3.7
cd "$SRC"

# Everything this script writes lives under mayhem/build/ (gitignored by upstream's `build/` rule),
# plus the fuzz binaries at /mayhem/<target>.
OUT="$SRC/mayhem/build"
mkdir -p "$OUT/gen"
# update_check.cpp includes the CMake-generated pt_version.h (cmake/version.cmake); write the same form.
printf '#pragma once\n#define PT_VERSION "0.0.0"\n#define PT_UPDATE_MANIFEST_URL ""\n' > "$OUT/gen/pt_version.h.tmp"
cmp -s "$OUT/gen/pt_version.h.tmp" "$OUT/gen/pt_version.h" 2>/dev/null && rm -f "$OUT/gen/pt_version.h.tmp" \
    || mv "$OUT/gen/pt_version.h.tmp" "$OUT/gen/pt_version.h"

SOURCES=(
  core/json core/log core/strcode core/pathcode core/memory_status
  fs/mod_level fs/mods fs/fpk fs/fox_crypt fs/qar fs/data_trace fs/io_stats
  platform/os platform/update_check platform/ed25519 platform/http
  data/fox2
  anim/gani anim/motion_data anim/demo_file anim/rig anim/anim_codec anim/skeleton anim/help_bones anim/sim_physics
  audio/wwise_bank audio/wem audio/wwise_vorbis audio/sound_package audio/subtitles
  ui/uif ui/ffnt ui/unicode_font ui/uigb ui/uia ui/uilb ui/lang_file
  vfx/vfx_file vfx/vfx_schema vfx/vfx_lensflare
  script/mod_lua
  assets/gltf_model assets/fmdl assets/ftex assets/geom fs/vfs fs/psarc
)
DEFS="-DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_RADIANS -DNOMINMAX -DVK_NO_PROTOTYPES"
INCS="-I$SRC/src -I$GLM/include -I$LUA/include -I$VOLK/include -I$VMA/include -I$VKH/include -I$OUT/gen -I$VORBIS/include"

# build_lib <dir> <flags...>: compile SOURCES into <dir>/libpt_parsers.a
build_lib() {
  local dir="$1"; shift
  mkdir -p "$dir/obj"
  local objs=() s
  for s in "${SOURCES[@]}"; do objs+=("$dir/obj/${s//\//_}.o"); done
  local i=0
  for s in "${SOURCES[@]}"; do
    # shellcheck disable=SC2086
    $CXX -std=c++20 "$@" $DEFS $INCS -c "$SRC/src/engine/$s.cpp" -o "${objs[$i]}" &
    i=$((i + 1))
    while [ "$(jobs -rp | wc -l)" -ge "$MAYHEM_JOBS" ]; do wait -n; done
  done
  local job; for job in $(jobs -rp); do wait "$job"; done
  # (jobs that failed are caught here: every object must exist and be fresh)
  for s in "${objs[@]}"; do [ -s "$s" ] || { echo "build.sh: missing $s" >&2; exit 1; }; done
  rm -f "$dir/libpt_parsers.a"
  ar rcs "$dir/libpt_parsers.a" "${objs[@]}"
}

# 1) the parsers, instrumented (ASan+UBSan halting, DWARF 3, and libFuzzer's SanCov edge counters so
#    the fuzzer gets coverage feedback from the parser code, not only from the harness)
FUZZ_COV="-fsanitize=fuzzer-no-link"
rm -rf "$OUT/fuzz/obj"
# shellcheck disable=SC2086
build_lib "$OUT/fuzz" -O1 $SANITIZER_FLAGS $FUZZ_COV $DEBUG_FLAGS

# 2) each harness twice: libFuzzer binary + standalone reproducer
# shellcheck disable=SC2086
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$OUT/lsan_off.o"
# shellcheck disable=SC2086
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OUT/standalone_main.o"
for h in "$SRC"/mayhem/harnesses/fuzz_*.cpp; do
  name=$(basename "$h" .cpp)
  # shellcheck disable=SC2086
  $CXX -std=c++20 -O1 $SANITIZER_FLAGS $FUZZ_COV $DEBUG_FLAGS $DEFS $INCS -c "$h" -o "$OUT/$name.o"
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$OUT/$name.o" "$OUT/lsan_off.o" \
      "$OUT/fuzz/libpt_parsers.a" "$LUA/lib/liblua.a" "$VORBIS/lib/libpt_vorbis.a" -lz -ldl -lm -lpthread -o "$SRC/$name"
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "$OUT/$name.o" "$OUT/standalone_main.o" "$OUT/lsan_off.o" \
      "$OUT/fuzz/libpt_parsers.a" "$LUA/lib/liblua.a" "$VORBIS/lib/libpt_vorbis.a" -lz -ldl -lm -lpthread -o "$SRC/$name-standalone"
done

# 3) upstream's own unit tests for this code, with normal flags (no sanitizers)
rm -rf "$OUT/test/obj"
# shellcheck disable=SC2086
build_lib "$OUT/test" -O1 $COVERAGE_FLAGS
for t in mods_test test_strcode; do
  # shellcheck disable=SC2086
  $CXX -std=c++20 -O1 $COVERAGE_FLAGS $DEFS $INCS "$SRC/tests/$t.cpp" "$OUT/test/libpt_parsers.a" \
      "$LUA/lib/liblua.a" "$VORBIS/lib/libpt_vorbis.a" -lz -ldl -lm -lpthread -o "$OUT/test/pt_$t"
done
echo "build.sh: done"
