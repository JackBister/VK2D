#include "WatchFile.h"

#include <chrono>
#include <thread>

#include "Logging/Logger.h"

static const auto logger = Logger::Create("WatchFile");

#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <filesystem>
#include <sys/stat.h>

struct CallbackParam {
    std::wstring filename;
    time_t lastModified;
    HANDLE watchHandle;
    std::function<void()> callback;
    std::chrono::high_resolution_clock::time_point lastCall;

    CallbackParam(std::wstring filename, time_t lastModified, HANDLE watchHandle, std::function<void()> callback)
        : filename(filename), lastModified(lastModified), watchHandle(watchHandle), callback(callback),
          lastCall(std::chrono::high_resolution_clock::now())
    {
    }
};

void WatchFileCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    auto arg = (CallbackParam *)lpParameter;
    auto now = std::chrono::high_resolution_clock::now();
    struct _stat statResult;
    _wstat(arg->filename.c_str(), &statResult);

    if (statResult.st_mtime <= arg->lastModified) {
        FindNextChangeNotification(arg->watchHandle);
        return;
    }
    arg->lastModified = statResult.st_mtime;
    // Throttling, the callback gets called multiple times per change for some reason and the user should only get one
    // callback per change
    if (now - arg->lastCall < std::chrono::seconds(1)) {
        FindNextChangeNotification(arg->watchHandle);
        return;
    }
    arg->lastCall = now;
    // TODO: This is pretty ugly but fopening the file immediately seems to get you stale data?
    // What I really want is a debounce where only the last notification for one file triggers the callback but that's
    // too annoying to implement right now
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    arg->callback();
    FindNextChangeNotification(arg->watchHandle);
}

void WatchFile_Windows(std::wstring filename, std::function<void()> onChange)
{
    std::filesystem::path path(filename);
    // You cannot use FindFirstChangeNotification on an individual file, so we look at the parent path and then stat the
    // file in the callback.
    auto absolutePath = std::filesystem::absolute(path).parent_path();

    auto watchHandle = FindFirstChangeNotification(absolutePath.c_str(), false, FILE_NOTIFY_CHANGE_LAST_WRITE);
    HANDLE newWaitObject;

    struct _stat statResult;
    _wstat(filename.c_str(), &statResult);

    void * callbackArgs = new CallbackParam(filename, statResult.st_mtime, watchHandle, onChange);
    auto registerResult = RegisterWaitForSingleObject(
        &newWaitObject, watchHandle, WatchFileCallback, callbackArgs, INFINITE, WT_EXECUTEDEFAULT);

    if (registerResult == 0) {
        logger.Error("Couldn't register file watcher, error code {}", GetLastError());
    }
}

#endif

void WatchFile(std::string fileName, std::function<void()> onChange)
{
#ifdef _WIN64
    WatchFile_Windows(std::wstring(fileName.begin(), fileName.end()), onChange);
#endif
}
