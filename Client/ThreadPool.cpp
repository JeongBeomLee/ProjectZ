#include "pch.h"
#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threadCount)
    : m_stop(false)
{
    // 작업자 스레드 생성
    for (size_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back(&ThreadPool::WorkerThread, this);
    }
}

ThreadPool::~ThreadPool() 
{
    Shutdown();
}

void ThreadPool::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }
	// 모든 스레드에게 종료를 알림
    m_condition.notify_all();

    // 모든 작업자 스레드 종료 대기
    for (std::thread& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

size_t ThreadPool::GetPendingJobCount() const
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_jobs.size();
}

const ThreadStats& ThreadPool::GetThreadStats(std::thread::id threadId)
{
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_threadStats[threadId];
}

std::vector<std::thread::id> ThreadPool::GetThreadIds() const 
{
    std::vector<std::thread::id> ids;
    std::lock_guard<std::mutex> lock(m_statsMutex);
    for (const auto& pair : m_threadStats) {
        ids.push_back(pair.first);
    }
    return ids;
}

void ThreadPool::DumpStats() const 
{
    std::lock_guard<std::mutex> lock(m_statsMutex);

    std::stringstream ss;
    ss << "\n=== Thread Pool Statistics ===\n";
    ss << "Total Threads: " << m_workers.size() << "\n";
    ss << "Pending Jobs: " << GetPendingJobCount() << "\n";
    ss << "Total Jobs Processed: " << m_totalJobsProcessed << "\n\n";

    for (const auto& [threadId, stats] : m_threadStats) {
        ss << "Thread " << threadId << ":\n";
        ss << "  Completed Jobs: " << stats.completedJobs << "\n";
        ss << "  Failed Jobs: " << stats.failedJobs << "\n";
        ss << "  Average Execution Time: "
            << (stats.completedJobs ? (stats.totalExecutionTime / stats.completedJobs) : 0)
            << " microseconds\n";

#ifdef _DEBUG
        auto it = m_currentJobs.find(threadId);
        if (it != m_currentJobs.end()) {
            auto currentTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - it->second.startTime).count();

            ss << "  Current Job: " << it->second.name
                << " (Running for " << duration << "ms)\n";
        }
#endif
        ss << "\n";
    }

	OutputDebugStringA(ss.str().c_str());
	//std::cout << ss.str() << std::endl;
}

void ThreadPool::LogThreadError(const std::string& errorMsg) 
{
    std::stringstream ss;
    ss << "Thread " << std::this_thread::get_id() << ": " << errorMsg << "\n";
    OutputDebugStringA(ss.str().c_str());
    //std::cout << ss.str() << std::endl;
}

void ThreadPool::WorkerThread() 
{
    auto threadId = std::this_thread::get_id();
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_threadStats[threadId] = ThreadStats{};
    }

    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

			// 작업이 없으면 대기
            m_condition.wait(lock, [this]() -> bool {
                return !m_jobs.empty() || m_stop;
                });

			// 작업이 종료되거나 없으면 스레드 종료
            if (m_stop && m_jobs.empty()) {
                return;
            }

            job = std::move(m_jobs.front());
            m_jobs.pop();
        }

        job();
        ++m_totalJobsProcessed;
    }
}
