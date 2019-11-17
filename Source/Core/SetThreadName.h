#pragma once

#include <string>
#include <thread>

void SetThreadName(std::thread::id id, std::string const & name);
