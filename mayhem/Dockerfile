# syntax=docker/dockerfile:1.7
#
# Commit-image Dockerfile (SMALL form, PREFERRED) — builds ON TOP of the org C/C++ base
# image (ghcr.io/savantenvs/base). The base already provides debian + clang/llvm +
# libFuzzer/afl++ + the identities + the build-contract ENV, so this only adds the
# per-commit build (built on top of the prebuilt base, rather than building the toolchain in-image).
#
# Build (from the repo root):
#   DOCKER_BUILDKIT=1 docker build -f mayhem/Dockerfile \
#       --build-arg COMMIT=$(git rev-parse HEAD) \
#       [--build-arg SOURCE_DATE_EPOCH=$(date +%s)] \
#       -t mytag .

# The org base, pinned to a fixed DATE-tagged base (YYYYMMDD; shared by all integrations
# minted against it). Use the current dated tag; pin by digest in production: ...base@sha256:<digest>.
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
# SOURCE_DATE_EPOCH stays an ARG (available to build.sh); do NOT bake it as an ENV — an
# empty value breaks clang ("must be a non-negative decimal integer") both here and for
# any in-image rebuild. build.sh unsets it when empty.
ARG SOURCE_DATE_EPOCH

# Sanitizer override: the base exports SANITIZER_FLAGS (ASan+UBSan, halting) as the default.
# To change it for a build (e.g. ASan-only, or add MSan): --build-arg SANITIZER_FLAGS="...".
# Declare it WITHOUT a default (no '='): an undefaulted ARG lets the base ENV flow through to
# build.sh when not passed, and an explicit --build-arg overrides it. (An empty default — `=` —
# would instead shadow the base ENV and silently wipe the sanitizers.)
ARG SANITIZER_FLAGS
# Coverage (optional, off by default): instrument the TEST build to measure how much of the project
# the suite exercises — --build-arg COVERAGE_FLAGS="-fprofile-instr-generate -fcoverage-mapping".
# Empty by default (no effect on the normal oracle build); build.sh appends it to the test build only.
ARG COVERAGE_FLAGS=
# Build parallelism (optional). build.sh falls back to nproc when unset: --build-arg MAYHEM_JOBS=8
ARG MAYHEM_JOBS=

# If mayhem/build.sh needs EXTRA apt packages, install them as root before the build
# (re-pin to SOURCE_DATE_EPOCH first for reproducibility — repro-sources-list.sh ships
# in the base):
#   USER root
#   RUN [ -n "$SOURCE_DATE_EPOCH" ] && repro-sources-list.sh || true ; \
#       apt-get update && apt-get install -yq --no-install-recommends <pkgs> \
#    && rm -rf /var/lib/apt/lists/*
#   USER mayhem

# Anything the upstream build would download, or a library apt lacks or ships at the wrong version, is
# built once from pinned source into a static prefix under /opt/toolchains/deps/<name>-<version>/
# (root-owned, world-readable, no .so); build.sh just points at it, e.g. via PKG_CONFIG_PATH
# (SPEC §6.2 item 19). The source stays in $d/src and the build runs out of tree with -gdwarf-3, so
# backtraces show /opt/toolchains/deps/... paths and the harness binary stays DWARF <= 3. Example:
#   USER root
#   ARG JANSSON_VER=2.14.1
#   ARG JANSSON_SHA256=<sha256 of the tarball>
#   RUN d=/opt/toolchains/deps/jansson-${JANSSON_VER} && mkdir -p "$d/src" /tmp/dep-build \
#    && curl -fsSL -o /tmp/dep.tgz \
#         https://github.com/akheron/jansson/releases/download/v${JANSSON_VER}/jansson-${JANSSON_VER}.tar.gz \
#    && echo "${JANSSON_SHA256}  /tmp/dep.tgz" | sha256sum -c - \
#    && tar -xzf /tmp/dep.tgz --strip-components=1 -C "$d/src" && cd /tmp/dep-build \
#    && "$d/src/configure" --prefix="$d" --disable-shared --enable-static CFLAGS="-O2 -g -gdwarf-3" \
#    && make -j"$(nproc)" install \
#    && cd / && rm -rf /tmp/dep-build /tmp/dep.tgz && chmod -R a+rX "$d"
#   USER mayhem

# Dependencies upstream's CMake build FetchContent-downloads (cmake/Dependencies.cmake), pinned to the
# same versions and cached under /opt/toolchains/deps (SPEC §6.2 item 19) so build.sh runs offline:
#   glm 1.0.3 (header-only math library every parser header includes)
#   Lua 5.1.5 (the sandboxed mod-script host, src/engine/script/mod_lua.cpp), built as a static lib
USER root
ARG GLM_VER=1.0.3
ARG GLM_SHA256=6775e47231a446fd086d660ecc18bcd076531cfedd912fbd66e576b118607001
ARG LUA_VER=5.1.5
ARG LUA_SHA256=2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333
RUN d=/opt/toolchains/deps/glm-${GLM_VER} && mkdir -p "$d/src" \
 && curl -fsSL -o /tmp/glm.tgz https://github.com/g-truc/glm/archive/refs/tags/${GLM_VER}.tar.gz \
 && echo "${GLM_SHA256}  /tmp/glm.tgz" | sha256sum -c - \
 && tar -xzf /tmp/glm.tgz --strip-components=1 -C "$d/src" \
 && mkdir -p "$d/include" && cp -r "$d/src/glm" "$d/include/glm" \
 && rm -f /tmp/glm.tgz && chmod -R a+rX,go-w "$d"
RUN d=/opt/toolchains/deps/lua-${LUA_VER} && mkdir -p "$d/src" "$d/include" "$d/lib" /tmp/lua-build \
 && curl -fsSL -o /tmp/lua.tgz https://www.lua.org/ftp/lua-${LUA_VER}.tar.gz \
 && echo "${LUA_SHA256}  /tmp/lua.tgz" | sha256sum -c - \
 && tar -xzf /tmp/lua.tgz --strip-components=1 -C "$d/src" \
 && cd /tmp/lua-build \
 && for f in "$d"/src/src/*.c; do case "$(basename "$f")" in lua.c|luac.c|print.c) continue;; esac; \
      clang -O2 -g -gdwarf-3 -DLUA_USE_POSIX -c "$f" -o "$(basename "$f" .c).o" || exit 1; done \
 && ar rcs "$d/lib/liblua.a" ./*.o \
 && cp "$d"/src/src/lua.h "$d"/src/src/luaconf.h "$d"/src/src/lualib.h "$d"/src/src/lauxlib.h "$d/include/" \
 && cd / && rm -rf /tmp/lua-build /tmp/lua.tgz && chmod -R a+rX,go-w "$d"

# Header-only Vulkan stack the model headers include (engine/render/mesh.h -> render/vk.h), same pins as
# cmake/Dependencies.cmake: volk 1.4.350, VulkanMemoryAllocator v3.4.0, Vulkan-Headers v1.4.350.
ARG VOLK_SHA256=e47c6efe5294bb03729a976b385864bba5daf46ec60f0bdea11e1d1446345f9a
ARG VMA_SHA256=822aa850c6ce77346ae96a8a1d351d52e77e85929f35363849a0a4e638e0a2a1
ARG VKH_SHA256=6dd105e5cc7ddab6e7b611ae2c1872740d1727557cc8bf9daf13d6de1e4b3999
RUN set -e; fetch() { d=/opt/toolchains/deps/$1; mkdir -p "$d/src" "$d/include"; \
      curl -fsSL -o /tmp/dep.tgz "$2"; echo "$3  /tmp/dep.tgz" | sha256sum -c -; \
      tar -xzf /tmp/dep.tgz --strip-components=1 -C "$d/src"; rm -f /tmp/dep.tgz; }; \
    fetch volk-1.4.350 https://github.com/zeux/volk/archive/refs/tags/1.4.350.tar.gz "$VOLK_SHA256"; \
    cp /opt/toolchains/deps/volk-1.4.350/src/volk.h /opt/toolchains/deps/volk-1.4.350/include/; \
    fetch vma-3.4.0 https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.4.0.tar.gz "$VMA_SHA256"; \
    cp -r /opt/toolchains/deps/vma-3.4.0/src/include/. /opt/toolchains/deps/vma-3.4.0/include/; \
    fetch vulkan-headers-1.4.350 https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.4.350.tar.gz "$VKH_SHA256"; \
    cp -r /opt/toolchains/deps/vulkan-headers-1.4.350/src/include/. /opt/toolchains/deps/vulkan-headers-1.4.350/include/; \
    chmod -R a+rX,go-w /opt/toolchains/deps

# Wwise Vorbis decoding (src/engine/audio/wem.cpp + wwise_vorbis.cpp), same pins and same pt_vorbis
# assembly as cmake/Dependencies.cmake: ogg v1.3.5 + vorbis v1.3.7 decoder sources compiled into one static
# libpt_vorbis.a, plus ww2ogg's packed_codebooks_aoTuV_603.bin (upstream's SHA256) turned into the same
# wwise_codebooks.c (pt_wwise_codebooks / pt_wwise_codebooks_size). Uninstrumented dependency.
ARG OGG_SHA256=f6f1b04cfa4e98b70ffe775d5e302d9c6b98541f05159af6de2d6617817ed7d6
ARG VORBIS_SHA256=270c76933d0934e42c5ee0a54a36280e2d87af1de3cc3e584806357e237afd13
ARG WW2OGG_COMMIT=14ed9b0dd62e815a38702b5f03c57006cbe2501b
ARG CODEBOOKS_SHA256=00a93eab267d281401b1efd54e888a2e183299b9e6c446c48d09f701a89d9d27
RUN set -e; d=/opt/toolchains/deps/vorbis-1.3.7; mkdir -p "$d/ogg-src" "$d/src" "$d/include/ogg" "$d/lib" /tmp/vb; \
    curl -fsSL -o /tmp/ogg.tgz https://github.com/xiph/ogg/archive/refs/tags/v1.3.5.tar.gz; \
    echo "${OGG_SHA256}  /tmp/ogg.tgz" | sha256sum -c -; tar -xzf /tmp/ogg.tgz --strip-components=1 -C "$d/ogg-src"; \
    curl -fsSL -o /tmp/vorbis.tgz https://github.com/xiph/vorbis/archive/refs/tags/v1.3.7.tar.gz; \
    echo "${VORBIS_SHA256}  /tmp/vorbis.tgz" | sha256sum -c -; tar -xzf /tmp/vorbis.tgz --strip-components=1 -C "$d/src"; \
    curl -fsSL -o "$d/packed_codebooks_aoTuV_603.bin" \
      "https://raw.githubusercontent.com/hcs64/ww2ogg/${WW2OGG_COMMIT}/packed_codebooks_aoTuV_603.bin"; \
    echo "${CODEBOOKS_SHA256}  $d/packed_codebooks_aoTuV_603.bin" | sha256sum -c -; \
    { printf '#include <stddef.h>\nconst unsigned char pt_wwise_codebooks[] = {'; \
      od -An -v -tx1 "$d/packed_codebooks_aoTuV_603.bin" | sed 's/ \([0-9a-f][0-9a-f]\)/0x\1,/g'; \
      printf '};\nconst size_t pt_wwise_codebooks_size = %d;\n' "$(stat -c%s "$d/packed_codebooks_aoTuV_603.bin")"; \
    } > "$d/wwise_codebooks.c"; \
    cp "$d"/ogg-src/include/ogg/ogg.h "$d"/ogg-src/include/ogg/os_types.h "$d/include/ogg/"; \
    printf '#ifndef __CONFIG_TYPES_H__\n#define __CONFIG_TYPES_H__\n#include <stdint.h>\ntypedef int16_t ogg_int16_t;\ntypedef uint16_t ogg_uint16_t;\ntypedef int32_t ogg_int32_t;\ntypedef uint32_t ogg_uint32_t;\ntypedef int64_t ogg_int64_t;\ntypedef uint64_t ogg_uint64_t;\n#endif\n' \
      > "$d/include/ogg/config_types.h"; \
    cp -r "$d/src/include/vorbis" "$d/include/vorbis"; \
    cd /tmp/vb; \
    for f in "$d/ogg-src/src/bitwise.c" "$d/ogg-src/src/framing.c" "$d/wwise_codebooks.c" \
             $(for n in mdct smallft block envelope window lsp lpc analysis synthesis psy info floor1 floor0 res0 \
                        mapping0 registry codebook sharedbook lookup bitrate; do echo "$d/src/lib/$n.c"; done); do \
      clang -O2 -g -gdwarf-3 -w -I"$d/include" -I"$d/src/lib" -c "$f" -o "$(basename "$f" .c).o"; done; \
    ar rcs "$d/lib/libpt_vorbis.a" ./*.o; \
    cd / && rm -rf /tmp/vb /tmp/ogg.tgz /tmp/vorbis.tgz && chmod -R a+rX,go-w "$d"

# Source enters from the build CONTEXT (the CI checkout): the FULL repo (incl. .git
# history) is copied to /mayhem as `mayhem`; build.sh builds in place.
USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

# Anything a Dockerfile step materializes into the tree (downloaded fixtures, generated headers,
# vendored deps) MUST be committed or (re)created by build.sh: rlenv's patch grader runs
# `git clean -ffdX` before every graded build, so a gitignored, untracked file is gone by then and
# every honest patch scores zero (SPEC §6.2 item 16; verify-repo.sh checks it).
#
# OPTIONAL compile-cache pre-fill (SPEC §6.2 item 18). Bases minted with /opt/mayhem/ccache/prefill
# run the build under rlenv's pinned ccache, so the patch grader's first (otherwise cold) rebuild is
# warm — this is what keeps large C/C++ targets inside the build time limit. Older bases fall back to
# the plain build; a missing or stale pre-fill only costs a cold cache. Never enable ccache in
# build.sh or via ENV/PATH, and keep the compiler resolving by name (CC=clang) or nothing is cached.
RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

# Run the project's functional tests at build time IF the repo ships them (mayhem/test.sh is
# optional — omit it only when the repo has no tests meaningful for RL). A failing suite fails the
# image build, so the commit image is guaranteed to build AND pass its own tests.
RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh — repo ships no functional tests"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
