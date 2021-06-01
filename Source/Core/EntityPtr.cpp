#include "EntityPtr.h"

#include <sstream>

#include "Core/EntityManager.h"

Entity * EntityPtr::Get() const
{
    if (!entityManager) {
        return nullptr;
    }
    return entityManager->Get(*this);
}

std::string EntityPtr::ToString() const
{
    std::stringstream ss;
    ss << "EntityPtr{\n"
       << "\tentityManager=" << entityManager << ",\n"
       << "\tindex=" << index << ",\n"
       << "\tgeneration=" << generation << ",\n"
       << '}';
    return ss.str();
}