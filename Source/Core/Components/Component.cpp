#include "Component.h"

#include "Logging/Logger.h"

static auto const logger = Logger::Create("Component");

REFLECT_STRUCT_BEGIN(Component)
REFLECT_STRUCT_END()

Component::~Component() {}

void Component::LogMissingEntity() const
{
    if (!entity) {
        logger.Warn("Missing entity for component={}, entityPtr={}", type, entity.ToString());
    }
}
