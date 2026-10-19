#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <string_view>
#include <cstring>
#include <cmath>
#include <span>

namespace OpenCraft::Net {

class ByteWriter {
public:
    ByteWriter() = default;
    explicit ByteWriter(size_t reserveBytes) { m_buffer.reserve(reserveBytes); }

    void writeUint8(uint8_t val) { m_buffer.push_back(val); }

    void writeInt8(int8_t val) { m_buffer.push_back(static_cast<uint8_t>(val)); }

    void writeUint16(uint16_t val) {
        m_buffer.push_back(static_cast<uint8_t>(val & 0xFF));
        m_buffer.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    }

    void writeInt16(int16_t val) { writeUint16(static_cast<uint16_t>(val)); }

    void writeUint32(uint32_t val) {
        m_buffer.push_back(static_cast<uint8_t>(val & 0xFF));
        m_buffer.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        m_buffer.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
        m_buffer.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
    }

    void writeInt32(int32_t val) { writeUint32(static_cast<uint32_t>(val)); }

    void writeUint64(uint64_t val) {
        for (int i = 0; i < 8; ++i) {
            m_buffer.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
        }
    }

    void writeInt64(int64_t val) { writeUint64(static_cast<uint64_t>(val)); }

    void writeFloat(float val) {
        static_assert(sizeof(float) == sizeof(uint32_t));
        uint32_t bits = 0;
        std::memcpy(&bits, &val, sizeof(bits));
        writeUint32(bits);
    }

    void writeString(std::string_view str) {
        uint16_t len = static_cast<uint16_t>(std::min<size_t>(str.size(), 65535));
        writeUint16(len);
        if (len > 0) {
            const auto* ptr = reinterpret_cast<const uint8_t*>(str.data());
            m_buffer.insert(m_buffer.end(), ptr, ptr + len);
        }
    }

    void writeBytes(const void* data, size_t size) {
        if (!data || size == 0) return;
        const auto* ptr = reinterpret_cast<const uint8_t*>(data);
        m_buffer.insert(m_buffer.end(), ptr, ptr + size);
    }

    const std::vector<uint8_t>& getBuffer() const noexcept { return m_buffer; }
    std::vector<uint8_t>& getBuffer() noexcept { return m_buffer; }
    const uint8_t* data() const noexcept { return m_buffer.data(); }
    size_t size() const noexcept { return m_buffer.size(); }
    void clear() noexcept { m_buffer.clear(); }

private:
    std::vector<uint8_t> m_buffer;
};

class ByteReader {
public:
    ByteReader(const void* data, size_t size)
        : m_data(reinterpret_cast<const uint8_t*>(data)),
          m_size(size),
          m_offset(0) {}

    explicit ByteReader(std::span<const uint8_t> span) : m_data(span.data()), m_size(span.size()), m_offset(0) {}

    bool hasRemaining(size_t bytes = 1) const noexcept { return (m_offset + bytes <= m_size); }

    size_t remainingBytes() const noexcept { return (m_offset <= m_size) ? (m_size - m_offset) : 0; }

    size_t offset() const noexcept { return m_offset; }
    size_t size() const noexcept { return m_size; }

    bool readUint8(uint8_t& out) noexcept {
        if (!hasRemaining(1)) return false;
        out = m_data[m_offset++];
        return true;
    }

    bool readInt8(int8_t& out) noexcept {
        uint8_t u = 0;
        if (!readUint8(u)) return false;
        out = static_cast<int8_t>(u);
        return true;
    }

    bool readUint16(uint16_t& out) noexcept {
        if (!hasRemaining(2)) return false;
        out = static_cast<uint16_t>(m_data[m_offset]) | (static_cast<uint16_t>(m_data[m_offset + 1]) << 8);
        m_offset += 2;
        return true;
    }

    bool readInt16(int16_t& out) noexcept {
        uint16_t u = 0;
        if (!readUint16(u)) return false;
        out = static_cast<int16_t>(u);
        return true;
    }

    bool readUint32(uint32_t& out) noexcept {
        if (!hasRemaining(4)) return false;
        out = static_cast<uint32_t>(m_data[m_offset]) | (static_cast<uint32_t>(m_data[m_offset + 1]) << 8) |
              (static_cast<uint32_t>(m_data[m_offset + 2]) << 16) | (static_cast<uint32_t>(m_data[m_offset + 3]) << 24);
        m_offset += 4;
        return true;
    }

    bool readInt32(int32_t& out) noexcept {
        uint32_t u = 0;
        if (!readUint32(u)) return false;
        out = static_cast<int32_t>(u);
        return true;
    }

    bool readUint64(uint64_t& out) noexcept {
        if (!hasRemaining(8)) return false;
        out = 0;
        for (int i = 0; i < 8; ++i) {
            out |= (static_cast<uint64_t>(m_data[m_offset + i]) << (i * 8));
        }
        m_offset += 8;
        return true;
    }

    bool readInt64(int64_t& out) noexcept {
        uint64_t u = 0;
        if (!readUint64(u)) return false;
        out = static_cast<int64_t>(u);
        return true;
    }

    bool readFloat(float& out) noexcept {
        uint32_t bits = 0;
        if (!readUint32(bits)) return false;
        std::memcpy(&out, &bits, sizeof(out));
        return std::isfinite(out);
    }

    bool readString(std::string& out, size_t maxLen = 4096) {
        uint16_t len = 0;
        if (!readUint16(len)) return false;
        if (len > maxLen || !hasRemaining(len)) return false;
        out.assign(reinterpret_cast<const char*>(m_data + m_offset), len);
        m_offset += len;
        return true;
    }

    bool readBytes(void* dest, size_t count) noexcept {
        if (!dest || !hasRemaining(count)) return false;
        std::memcpy(dest, m_data + m_offset, count);
        m_offset += count;
        return true;
    }

    void skip(size_t count) noexcept { m_offset = std::min(m_offset + count, m_size); }

private:
    const uint8_t* m_data = nullptr;
    size_t m_size = 0;
    size_t m_offset = 0;
};

} // namespace OpenCraft::Net
