#include "Core/eventarg.h"

#include "Logging/Logger.h"

static const auto logger = Logger::Create("EventArg");

// We need to do this both in the move assignment and in destructor
void EventArg::Delete()
{
    if (type == Type::STRING && asString != nullptr) {
        delete asString;
    }
    if (type == Type::EVENTARGS && asEventArgs != nullptr) {
        delete asEventArgs;
    }
    if (type == Type::VECTOR && asVector != nullptr) {
        delete asVector;
    }
}

EventArg::~EventArg()
{
    Delete();
}

EventArg::EventArg() {}

EventArg::EventArg(EventArg const & ea) : type(ea.type)
{
    switch (type) {
    case Type::STRING:
        asString = new std::string(*ea.asString);
        break;
    case Type::INT:
        asInt = ea.asInt;
        break;
    case Type::FLOAT:
        asFloat = ea.asFloat;
        break;
    case Type::DOUBLE:
        asDouble = ea.asDouble;
        break;
    case Type::POINTER:
        asPointer = ea.asPointer;
        break;
    case Type::EVENTARGS:
        asEventArgs = new EventArgs(*ea.asEventArgs);
        break;
    case Type::VECTOR:
        asVector = new std::vector<EventArg>(*ea.asVector);
        break;
    case Type::ENTITY_POINTER:
        asEntityPtr = ea.asEntityPtr;
        break;
    default:
        logger.Error("Unknown EventArg::type {}", type);
    }
}

EventArg::EventArg(EventArg && ea) : type(ea.type)
{
    switch (type) {
    case Type::STRING:
        asString = ea.asString;
        ea.asString = nullptr;
        break;
    case Type::INT:
        asInt = ea.asInt;
        break;
    case Type::FLOAT:
        asFloat = ea.asFloat;
        break;
    case Type::DOUBLE:
        asDouble = ea.asDouble;
        break;
    case Type::POINTER:
        asPointer = ea.asPointer;
        break;
    case Type::EVENTARGS:
        asEventArgs = ea.asEventArgs;
        ea.asEventArgs = nullptr;
        break;
    case Type::VECTOR:
        asVector = ea.asVector;
        ea.asVector = nullptr;
        break;
    case Type::ENTITY_POINTER:
        asEntityPtr = ea.asEntityPtr;
        ea.asEntityPtr = EntityPtr();
        break;
    default:
        logger.Error("Unknown EventArg::type {}", type);
    }
}

EventArg & EventArg::operator=(EventArg const & ea)
{
    EventArg tmp(ea);
    *this = std::move(tmp);
    return *this;
}

EventArg & EventArg::operator=(EventArg && ea)
{
    Delete();
    type = ea.type;
    switch (type) {
    case Type::STRING:
        asString = ea.asString;
        ea.asString = nullptr;
        break;
    case Type::INT:
        asInt = ea.asInt;
        break;
    case Type::FLOAT:
        asFloat = ea.asFloat;
        break;
    case Type::DOUBLE:
        asDouble = ea.asDouble;
        break;
    case Type::POINTER:
        asPointer = ea.asPointer;
        break;
    case Type::EVENTARGS:
        asEventArgs = ea.asEventArgs;
        ea.asEventArgs = nullptr;
        break;
    case Type::VECTOR:
        asVector = ea.asVector;
        ea.asVector = nullptr;
        break;
    case Type::ENTITY_POINTER:
        asEntityPtr = ea.asEntityPtr;
        ea.asEntityPtr = EntityPtr();
        break;
    default:
        logger.Error("Unknown EventArg::type {}", type);
    }
    return *this;
}

EventArg::EventArg(std::string s) : type(Type::STRING), asString(new std::string(s)) {}

EventArg::EventArg(char const * s) : type(Type::STRING), asString(new std::string(s)) {}
EventArg::EventArg(int i) : type(Type::INT), asInt(i) {}

EventArg::EventArg(float f) : type(Type::FLOAT), asFloat(f) {}

EventArg::EventArg(double d) : type(Type::DOUBLE), asDouble(d) {}

EventArg::EventArg(void * ptr) : type(Type::POINTER), asPointer(ptr) {}

EventArg::EventArg(EventArgs ea) : type(Type::EVENTARGS), asEventArgs(new EventArgs(ea)) {}

EventArg::EventArg(std::vector<EventArg> const & v) : type(Type::VECTOR), asVector(new std::vector<EventArg>(v)) {}

EventArg::EventArg(EntityPtr ent) : type(Type::ENTITY_POINTER), asEntityPtr(ent) {}
