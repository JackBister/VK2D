#pragma once

#include <functional>
#include <string>

// TODO: How do you "unwatch" a file?
void WatchFile(std::string fileName, std::function<void()> onChange);
