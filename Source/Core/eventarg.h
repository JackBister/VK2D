#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "Core/DllExport.h"
#include "Core/EntityPtr.h"

class EventArg;

typedef class std::unordered_map<std::string, EventArg> EventArgs;

/*
        An EventArg represents an argument to an OnEvent call.
        It can hold different types of values. The type of the contained value can be read from EventArg::type.
        The special case with this class is when it's holding a string value. In that case, it makes a copy of the
   passed string value using new. It does this because it needs a pointer for the union, but the user shouldn't need to
   think about making a pointer before creating the eventarg. This way, the user can hardcode strings as arguments.

        TODO: Make the union private and use getters? If you're an idiot you can break it right now by assigning to the
   union.
*/

class EAPI EventArg
{
public:
    enum Type { STRING, INT, FLOAT, DOUBLE, POINTER, EVENTARGS, VECTOR, ENTITY_POINTER };

    // For string pointer
    ~EventArg();
    // Necessary to work in initializer list for unordered_map
    EventArg();
    EventArg(EventArg const &);
    EventArg(EventArg &&);
    EventArg(std::string);
    EventArg(char const *);
    EventArg(int);
    EventArg(float);
    EventArg(double);
    EventArg(void *);
    EventArg(EventArgs);
    EventArg(std::vector<EventArg> const &);
    EventArg(EntityPtr);

    EventArg & operator=(EventArg const &);
    EventArg & operator=(EventArg && ea);

    Type type;
    union {
        std::string * asString;
        int asInt;
        float asFloat;
        double asDouble;
        void * asPointer;
        EventArgs * asEventArgs;
        std::vector<EventArg> * asVector;
        EntityPtr asEntityPtr;
    };

private:
    void Delete();
};
