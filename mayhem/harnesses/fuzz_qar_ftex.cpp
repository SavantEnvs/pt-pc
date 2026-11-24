// The game dump's .qar archives and the .ftex/.ftexs textures read from them: pt::QarArchive::Open
// (footer, entry table), then pt::LoadFtex (ftex header, mip table, .N.ftexs side files, chunked zlib
// mips) for one texture path and DecodeFtexLevel (BC1/BC2/BC3/BC5/R8/R32F/BGRA decoders) for each mip.
// QarArchive only opens filesystem paths, so each input is written to a fresh file under $TMPDIR and
// removed afterwards. Entries are keyed by PathCode64, so the seeds name kTexture's .ftex/.1.ftexs codes.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

#include "engine/assets/ftex.h"
#include "engine/fs/qar.h"

static constexpr const char* kTexture = "/Assets/tpp/fuzz/fuzz_tex.ftex";

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    const char* tmp = std::getenv("TMPDIR");
    std::string path = std::string(tmp && *tmp ? tmp : "/tmp") + "/pt_qar_XXXXXX";
    const int fd = mkstemp(path.data());
    if (fd < 0) return 0;
    size_t done = 0;
    while (done < size) {
        const ssize_t n = write(fd, data + done, size - done);
        if (n <= 0) break;
        done += static_cast<size_t>(n);
    }
    close(fd);
    {
        pt::QarArchive qar;
        if (done == size && qar.Open(path)) {
            (void)pt::ReadTextureFile(qar, pt::FtexStem(kTexture) + ".ftex");
            pt::FtexTexture tex;
            if (pt::LoadFtex(qar, kTexture, tex)) {
                (void)tex.Format();
                std::vector<uint8_t> rgba;
                for (uint32_t level = 0; level < tex.mip_count; ++level) pt::DecodeFtexLevel(tex, level, rgba);
            }
        }
    }
    unlink(path.c_str());
    return 0;
}
