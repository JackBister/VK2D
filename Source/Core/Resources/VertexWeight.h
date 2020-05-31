#pragma once

class VertexWeight
{
public:
    VertexWeight() = default;
    VertexWeight(size_t vertexIdx, float weight) : vertexIdx(vertexIdx), weight(weight) {}

    inline size_t GetVertexIdx() const { return vertexIdx; }
    inline float GetWeight() const { return weight; }

    size_t vertexIdx;
    float weight;
};