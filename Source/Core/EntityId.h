#pragma once

#include <string>

#include "Core/Reflect.h"
#include "Util/DllExport.h"

class EAPI EntityId
{
public:
    friend class std::hash<EntityId>;

    EntityId(std::string_view id) : id(id) {}

    auto operator<=>(EntityId const & rhs) const = default;

    std::string_view const ToString() const { return id; }

    REFLECT();

private:
    std::string id;
};

namespace std
{
template <>
struct hash<EntityId> {
    size_t operator()(EntityId const & id) const { return std::hash<std::string>()(id.id); }
};
}
