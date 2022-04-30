#pragma once

#include <format>
#include <sstream>

class BufferHandle;

class BufferSlice
{
public:
    BufferSlice() : buffer(nullptr), offset(offset), size(size) {}
    BufferSlice(BufferHandle * buffer, size_t offset, size_t size) : buffer(buffer), offset(offset), size(size) {}

    operator bool() const { return buffer != nullptr; }

    inline BufferHandle * GetBuffer() const { return buffer; }
    inline size_t GetOffset() const { return offset; }
    inline size_t GetSize() const { return size; }

private:
    BufferHandle * buffer;
    size_t offset;
    size_t size;
};

template <typename CharT>
struct std::formatter<BufferSlice, CharT> : std::formatter<std::string, CharT> {
    template <class FormatContext>
    auto format(BufferSlice b, FormatContext & ctx)
    {
        std::stringstream ss;
        ss << "BufferSlice{\n";
        ss << "\tbuffer=" << b.GetBuffer() << ",\n";
        ss << "\toffset=" << b.GetOffset() << ",\n";
        ss << "\tsize=" << b.GetSize() << "\n";
        ss << "}";
        return std::formatter<std::string, CharT>::format(ss.str(), ctx);
    }
};