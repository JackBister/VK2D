#pragma once

#include <format>
#include <optional>
#include <string>

enum class SerializedValueType { BOOL = 0, DOUBLE = 1, STRING = 2, OBJECT = 3, ARRAY = 4 };

std::optional<SerializedValueType> SerializedValueTypeFromString(std::string s);
std::string SerializedValueTypeToString(SerializedValueType t);

template <class CharT>
struct std::formatter<SerializedValueType, CharT> : std::formatter<std::string, CharT> {
    template <class FormatContext>
    auto format(SerializedValueType t, FormatContext & ctx)
    {
        return std::formatter<std::string, CharT>::format(SerializedValueTypeToString(t), ctx);
    }
};
