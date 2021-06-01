#pragma once

#include "Core/Deserializable.h"
#include "Core/EntityPtr.h"
#include "Core/Reflect.h"
#include "Core/eventarg.h"
#include "Util/DllExport.h"
#include "Util/HashedString.h"

class EntityManager;

/*
    This must be put in the .cpp file for the component.
*/
#define COMPONENT_IMPL(str, fn) DESERIALIZABLE_IMPL(str, fn)

class EAPI Component : public Deserializable
{
public:
    friend class EntityManager;

    virtual ~Component();

    virtual void OnEvent(HashedString name, EventArgs args = {}) = 0;

    EntityPtr entity;

    bool isActive = true;
    bool receiveTicks;

    REFLECT()
    REFLECT_INHERITANCE()
protected:
    void LogMissingEntity() const;
};
