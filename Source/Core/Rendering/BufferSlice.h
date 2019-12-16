#pragma once

class BufferHandle;

class BufferSlice
{
public:
    BufferSlice(BufferHandle * buffer, size_t offset, size_t size) : buffer(buffer), offset(offset), size(size) {}

    inline BufferHandle * GetBuffer() const { return buffer; }
    inline size_t GetOffset() const { return offset; }
    inline size_t GetSize() const { return size; }

private:
    BufferHandle * buffer;
    size_t offset;
    size_t size;
};
