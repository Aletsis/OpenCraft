#include "core/PngWriter.hpp"
#include "storage/WorldFormat.hpp"
#include <algorithm>
#include <fstream>

namespace OpenCraft::PngWriter {

static void putU32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v >> 24));
    out.push_back(static_cast<uint8_t>(v >> 16));
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v));
}

static void putChunk(std::vector<uint8_t>& out, const char type[4], const std::vector<uint8_t>& data) {
    putU32(out, static_cast<uint32_t>(data.size()));
    size_t crcStart = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    uint32_t crc = computeCRC32(std::span<const uint8_t>(out.data() + crcStart, out.size() - crcStart));
    putU32(out, crc);
}

std::vector<uint8_t> encodeRGBA(std::span<const uint8_t> rgba, uint32_t width, uint32_t height) {
    std::vector<uint8_t> out = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    if (width == 0 || height == 0 || rgba.size() < static_cast<size_t>(width) * height * 4) {
        return {};
    }

    std::vector<uint8_t> ihdr;
    putU32(ihdr, width);
    putU32(ihdr, height);
    ihdr.insert(ihdr.end(), {8, 6, 0, 0, 0}); // 8 bits, RGBA, deflate, filtro 0, sin entrelazado
    putChunk(out, "IHDR", ihdr);

    // Datos crudos: cada fila precedida por el byte de filtro 0
    const size_t rowBytes = static_cast<size_t>(width) * 4;
    std::vector<uint8_t> raw;
    raw.reserve((rowBytes + 1) * height);
    for (uint32_t y = 0; y < height; ++y) {
        raw.push_back(0);
        const uint8_t* row = rgba.data() + y * rowBytes;
        raw.insert(raw.end(), row, row + rowBytes);
    }

    // Flujo zlib con bloques deflate almacenados (máx. 65535 bytes cada uno)
    std::vector<uint8_t> z = {0x78, 0x01};
    size_t pos = 0;
    do {
        size_t len = std::min<size_t>(65535, raw.size() - pos);
        bool last = (pos + len == raw.size());
        z.push_back(last ? 1 : 0);
        z.push_back(static_cast<uint8_t>(len));
        z.push_back(static_cast<uint8_t>(len >> 8));
        z.push_back(static_cast<uint8_t>(~len));
        z.push_back(static_cast<uint8_t>((~len) >> 8));
        z.insert(z.end(), raw.begin() + static_cast<std::ptrdiff_t>(pos),
                 raw.begin() + static_cast<std::ptrdiff_t>(pos + len));
        pos += len;
    } while (pos < raw.size());

    uint32_t a = 1, b = 0;
    for (uint8_t byte : raw) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    putU32(z, (b << 16) | a);
    putChunk(out, "IDAT", z);
    putChunk(out, "IEND", {});
    return out;
}

bool writeRGBA(const std::filesystem::path& path, std::span<const uint8_t> rgba, uint32_t width, uint32_t height) {
    std::vector<uint8_t> png = encodeRGBA(rgba, width, height);
    if (png.empty()) return false;
    std::error_code ec;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    return static_cast<bool>(f);
}

} // namespace OpenCraft::PngWriter
