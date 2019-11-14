#pragma once

#include <cassert>
#include <functional>
#include <string>
#include <vector>

class CommandDefinition
{
public:
    CommandDefinition() { assert(false); }
    CommandDefinition(std::string name, std::string docString, int numParameters,
                      std::function<void(std::vector<std::string>)> executable)
        : docString(docString), name(name), numParameters(numParameters), executable(executable)
    {
    }

    inline void Execute(std::vector<std::string> args) const { executable(args); }
    inline std::string GetDocString() const { return docString; }
    inline std::string GetName() const { return name; }
    inline int GetNumParameters() const { return numParameters; }

private:
    std::string docString;
    std::string name;
    int numParameters;
    std::function<void(std::vector<std::string>)> executable;
};
