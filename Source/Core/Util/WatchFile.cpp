#include "WatchFile.h"

#include <chrono>

#include "Core/Logging/Logger.h"

static const auto logger = Logger::Create("WatchFile");

#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <filesystem>

struct CallbackParam {
    HANDLE watchHandle;
    std::function<void()> callback;
    std::chrono::high_resolution_clock::time_point lastCall;

    CallbackParam(HANDLE watchHandle, std::function<void()> callback)
        : watchHandle(watchHandle), callback(callback), lastCall(std::chrono::high_resolution_clock::now())
    {
    }
};

void WatchFileCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    auto arg = (CallbackParam *)lpParameter;
    auto now = std::chrono::high_resolution_clock::now();
    // Throttling, the callback gets called multiple times per change for some reason and the user should only get one
    // callback per change
    if (now - arg->lastCall < std::chrono::seconds(1)) {
        FindNextChangeNotification(arg->watchHandle);
        return;
    }
    arg->lastCall = now;
    arg->callback();
    FindNextChangeNotification(arg->watchHandle);
}

void WatchFile_Windows(std::wstring filename, std::function<void()> onChange)
{
    std::filesystem::path path(filename);
    auto absolutePath = std::filesystem::absolute(path).parent_path();

    auto watchHandle = FindFirstChangeNotification(absolutePath.c_str(), false, FILE_NOTIFY_CHANGE_LAST_WRITE);
    HANDLE newWaitObject;

    void * callbackArgs = new CallbackParam(watchHandle, onChange);
    auto registerResult = RegisterWaitForSingleObject(
        &newWaitObject, watchHandle, WatchFileCallback, callbackArgs, INFINITE, WT_EXECUTEDEFAULT);

    if (registerResult == 0) {
        logger->Errorf("Couldn't register file watcher, error code %x", GetLastError());
    }
}

#endif

void WatchFile(std::string fileName, std::function<void()> onChange)
{
#ifdef _WIN64
    WatchFile_Windows(std::wstring(fileName.begin(), fileName.end()), onChange);
#endif
}
