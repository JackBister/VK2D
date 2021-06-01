#include "Core/Components/Component.h"

#include "Core/Logging/Logger.h"

static auto const logger = Logger::Create("Component");

REFLECT_STRUCT_BEGIN(Component)
REFLECT_STRUCT_END()

Component::~Component() {}

void Component::LogMissingEntity() const
{
    if (!entity) {
        logger->Warnf("Missing entity for component=%s, entityPtr=%s", type.c_str(), entity.ToString().c_str());
    }
}
