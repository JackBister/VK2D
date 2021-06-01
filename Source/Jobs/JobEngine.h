#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Util/Queue.h"
#include "Util/Semaphore.h"

using JobId = size_t;

class JobEngine;

enum class JobPriority { LOW, MEDIUM, HIGH };
enum class JobState { NOT_SCHEDULED, WAITING, RUNNING, FINISHED };

class Job
{
public:
    friend class JobEngine;

    Job(std::vector<JobId> dependsOn, std::function<void()> fn);

private:
    JobState state;
    size_t scheduledOnFrame;

    std::vector<JobId> dependsOn;
    std::function<void()> fn;
};

class JobEngine
{
public:
    static JobEngine * GetInstance();

    JobEngine(uint32_t numThreads);

    JobId CreateJob(std::vector<JobId> dependsOn, std::function<void()> fn);
    void ScheduleJob(JobId id, JobPriority priority);

    uint32_t GetCurrentThreadIndex();
    void RegisterMainThread();

private:
    static JobEngine * instance;

    static void JobThread(uint32_t threadIdx, JobEngine * jobEngine, Queue<JobId>::Reader lowPriorityQueue,
                          Queue<JobId>::Reader mediumPriorityQueue, Queue<JobId>::Reader highPriorityQueue);

    bool AnyDependenciesUnfinished(JobId id);
    void EnqueueJob(JobId id, JobPriority priority);
    void RunJob(JobId id);

    Semaphore jobsWaiting;

    // TODO: Don't really want to lock the map, there is surely a smarter way.
    std::mutex jobsLock;
    std::unordered_map<JobId, Job> jobs;
    std::vector<std::thread> threads;
    std::unordered_map<std::thread::id, uint32_t> threadIdToIndex;

    Queue<JobId> lowPriorityQueue;
    Queue<JobId> mediumPriorityQueue;
    Queue<JobId> highPriorityQueue;

    std::vector<Queue<JobId>::Writer> lowPriorityWriters;
    std::vector<Queue<JobId>::Writer> mediumPriorityWriters;
    std::vector<Queue<JobId>::Writer> highPriorityWriters;
};
