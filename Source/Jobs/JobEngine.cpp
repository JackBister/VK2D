#include "JobEngine.h"

#include <optick/optick.h>

#include "Core/Logging/Logger.h"
#include "Util/SetThreadName.h"

static auto const logger = Logger::Create("JobEngine");

JobEngine * JobEngine::instance = nullptr;

void JobEngine::JobThread(uint32_t threadIdx, JobEngine * jobEngine, Queue<JobId>::Reader lowPriorityQueue,
                          Queue<JobId>::Reader mediumPriorityQueue, Queue<JobId>::Reader highPriorityQueue)
{
    std::string threadName("JobEngine-");
    threadName += std::to_string(threadIdx);
    SetThreadName(std::this_thread::get_id(), threadName);

    OPTICK_THREAD(threadName.c_str());

    while (true) {
        // Did I just make a lock free queue into a locked queue?
        // TODO: There is some kind of deadlock here where jobs are available in the queue but Wait() never returns
        jobEngine->jobsWaiting.Wait();
        auto high = highPriorityQueue.Pop();
        if (high.has_value()) {
            auto id = high.value();
            bool shouldRun = true;
            if (jobEngine->AnyDependenciesUnfinished(id)) {
                jobEngine->EnqueueJob(id, JobPriority::HIGH);
                shouldRun = false;
            }
            if (shouldRun) {
                jobEngine->RunJob(id);
            }
            continue;
        }
        auto medium = mediumPriorityQueue.Pop();
        if (medium.has_value()) {
            auto id = medium.value();
            bool shouldRun = true;
            if (jobEngine->AnyDependenciesUnfinished(id)) {
                jobEngine->EnqueueJob(id, JobPriority::HIGH);
                shouldRun = false;
            }
            if (shouldRun) {
                jobEngine->RunJob(id);
            }
            continue;
        }
        auto low = lowPriorityQueue.Pop();
        if (low.has_value()) {
            auto id = low.value();
            bool shouldRun = true;
            if (jobEngine->AnyDependenciesUnfinished(id)) {
                jobEngine->EnqueueJob(id, JobPriority::HIGH);
                shouldRun = false;
            }
            if (shouldRun) {
                jobEngine->RunJob(id);
            }
            continue;
        }
    }
}

Job::Job(std::vector<JobId> dependsOn, std::function<void()> fn)
    : state(JobState::NOT_SCHEDULED), scheduledOnFrame(0), dependsOn(dependsOn), fn(fn)
{
}

JobEngine * JobEngine::GetInstance()
{
    return JobEngine::instance;
}

JobEngine::JobEngine(uint32_t numThreads)
{
    for (uint32_t i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread::thread(JobEngine::JobThread,
                                              i,
                                              this,
                                              lowPriorityQueue.GetReader(),
                                              mediumPriorityQueue.GetReader(),
                                              highPriorityQueue.GetReader()));
        threadIdToIndex.insert({threads.back().get_id(), i});
        lowPriorityWriters.push_back(lowPriorityQueue.GetWriter());
        mediumPriorityWriters.push_back(mediumPriorityQueue.GetWriter());
        highPriorityWriters.push_back(highPriorityQueue.GetWriter());
    }

    JobEngine::instance = this;
}

JobId JobEngine::CreateJob(std::vector<JobId> dependsOn, std::function<void()> fn)
{
    OPTICK_EVENT()
    {
        std::lock_guard<std::mutex> lock(jobsLock);
        auto id = jobs.size();
        jobs.insert({id, Job(dependsOn, fn)});
        return id;
    }
}

void JobEngine::ScheduleJob(JobId id, JobPriority priority)
{
    OPTICK_EVENT()
    auto & job = jobs.at(id);
    for (JobId id : job.dependsOn) {
        auto dependedJob = jobs.at(id);
        if (dependedJob.state == JobState::NOT_SCHEDULED) {
            ScheduleJob(id, priority);
        }
    }
    job.state = JobState::WAITING;
    EnqueueJob(id, priority);
}

uint32_t JobEngine::GetCurrentThreadIndex()
{
    OPTICK_EVENT()
    auto id = std::this_thread::get_id();
    auto find = threadIdToIndex.find(id);
    if (find == threadIdToIndex.end()) {
        return 0;
    }
    return find->second;
}

void JobEngine::RegisterMainThread()
{
    auto thisIdx = threads.size();
    threadIdToIndex.insert({std::this_thread::get_id(), thisIdx});
    lowPriorityWriters.push_back(lowPriorityQueue.GetWriter());
    mediumPriorityWriters.push_back(mediumPriorityQueue.GetWriter());
    highPriorityWriters.push_back(highPriorityQueue.GetWriter());
}

bool JobEngine::AnyDependenciesUnfinished(JobId id)
{
    OPTICK_EVENT()
    std::lock_guard<std::mutex> lock(jobsLock);
    auto & job = jobs.at(id);
    for (auto const & dep : job.dependsOn) {
        if (jobs.at(dep).state != JobState::FINISHED) {
            return true;
        }
    }
    return false;
}

void JobEngine::EnqueueJob(JobId id, JobPriority priority)
{
    OPTICK_EVENT();
    auto threadIdx = GetCurrentThreadIndex();
    if (priority == JobPriority::HIGH) {
        highPriorityWriters[threadIdx].Push(std::move(id));
    } else if (priority == JobPriority::MEDIUM) {
        mediumPriorityWriters[threadIdx].Push(std::move(id));
    } else {
        lowPriorityWriters[threadIdx].Push(std::move(id));
    }
    jobsWaiting.Signal();
}

void JobEngine::RunJob(JobId id)
{
    OPTICK_EVENT();
    std::function<void()> fn;
    {
        OPTICK_EVENT("SetRunning")
        std::lock_guard<std::mutex> lock(jobsLock);
        auto & job = jobs.at(id);
        job.state = JobState::RUNNING;
        fn = job.fn;
    }
    fn();
    {
        OPTICK_EVENT("SetFinished")
        std::lock_guard<std::mutex> lock(jobsLock);
        auto & job = jobs.at(id);
        job.state = JobState::FINISHED;
    }
}
