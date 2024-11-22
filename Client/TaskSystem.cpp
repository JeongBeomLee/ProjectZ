#include "pch.h"
#include "TaskSystem.h"
#include "Logger.h"

std::atomic<uint64_t> TaskNode::s_nextId(0);

TaskSystem& TaskSystem::GetInstance() {
    static TaskSystem instance;
    return instance;
}

bool TaskSystem::Initialize(size_t threadCount) {
    if (m_isRunning) {
        return false;
    }

    m_isRunning = true;

    try {
        for (size_t i = 0; i < threadCount; ++i) {
            m_workerThreads.emplace_back(&TaskSystem::WorkerThread, this);
        }

        LOG_INFO("Task System initialized with {} worker threads", threadCount);
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize Task System: {}", e.what());
        Shutdown();
        return false;
    }
}

void TaskSystem::Shutdown() {
    m_isRunning = false;

    for (auto& thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    m_workerThreads.clear();

    std::unique_lock<std::shared_mutex> lock(m_tasksMutex);
    m_tasks.clear();

    LOG_INFO("Task System shut down");
}

bool TaskNode::IsReady() const {
    return m_remainingDependencies.load() == 0;
}

void TaskNode::Execute() {
    try {
        m_info.state = TaskState::Running;
        m_info.threadId = std::this_thread::get_id();
        m_info.startTime = std::chrono::steady_clock::now().time_since_epoch().count();

        m_function();

        m_info.state = TaskState::Completed;
    }
    catch (const std::exception& e) {
        m_info.state = TaskState::Failed;
        m_info.error = e.what();
        LOG_ERROR("Task '{}' failed: {}", m_info.name, e.what());
    }

    m_info.endTime = std::chrono::steady_clock::now().time_since_epoch().count();
}

void TaskSystem::WorkerThread() {
    while (m_isRunning) {
        std::shared_ptr<TaskNode> task;

        // 우선순위 큐에서 작업 가져오기
        if (m_highPriorityQueue.TryDequeue(task)) {
            // 높은 우선순위 작업 처리
        }
        else if (m_normalPriorityQueue.TryDequeue(task)) {
            // 보통 우선순위 작업 처리
        }
        else if (m_lowPriorityQueue.TryDequeue(task)) {
            // 낮은 우선순위 작업 처리
        }
        else {
            std::this_thread::yield();
            continue;
        }

        if (task && task->IsReady()) {
            task->Execute();
            ProcessCompletedTask(task);
            UpdateDependencies(task->GetId());
        }
    }
}

void TaskSystem::ScheduleTask(std::shared_ptr<TaskNode> task) {
    {
        std::unique_lock<std::shared_mutex> lock(m_tasksMutex);
        m_tasks[task->GetId().Get()] = task;
    }

    task->m_info.state = TaskState::Scheduled;

    // 우선순위에 따라 적절한 큐에 작업 추가
    switch (task->GetInfo().priority) {
    case TaskPriority::High:
        while (!m_highPriorityQueue.TryEnqueue(task)) {
            std::this_thread::yield();
        }
        break;
    case TaskPriority::Normal:
        while (!m_normalPriorityQueue.TryEnqueue(task)) {
            std::this_thread::yield();
        }
        break;
    case TaskPriority::Low:
        while (!m_lowPriorityQueue.TryEnqueue(task)) {
            std::this_thread::yield();
        }
        break;
    }
}

void TaskSystem::ProcessCompletedTask(const std::shared_ptr<TaskNode>& task) {
    if (task->GetInfo().state == TaskState::Failed) {
        m_failedTasks++;
    }

    m_totalTasksProcessed++;

    double executionTime =
        static_cast<double>(task->GetInfo().endTime - task->GetInfo().startTime);
    m_totalExecutionTime += executionTime;
}

void TaskSystem::UpdateDependencies(TaskId completedTaskId) {
    std::shared_lock<std::shared_mutex> lock(m_tasksMutex);

    for (const auto& [id, task] : m_tasks) {
        bool found = false;
        for (const auto& depId : task->m_dependencies) {
            if (depId.Get() == completedTaskId.Get()) {
                found = true;
                break;
            }
        }

        if (found) {
            if (task->m_remainingDependencies.fetch_sub(1) == 1) {
                ScheduleTask(task);
            }
        }
    }
}

void TaskSystem::WaitForTask(TaskId id) {
    while (!IsTaskComplete(id)) {
        std::this_thread::yield();
    }
}

void TaskSystem::WaitForAll() {
    while (true) {
        bool allCompleted = true;
        {
            std::shared_lock<std::shared_mutex> lock(m_tasksMutex);
            for (const auto& [id, task] : m_tasks) {
                if (task->GetInfo().state != TaskState::Completed &&
                    task->GetInfo().state != TaskState::Failed) {
                    allCompleted = false;
                    break;
                }
            }
        }

        if (allCompleted) {
            break;
        }

        std::this_thread::yield();
    }
}

bool TaskSystem::IsTaskComplete(TaskId id) const {
    std::shared_lock<std::shared_mutex> lock(m_tasksMutex);
    auto it = m_tasks.find(id.Get());
    if (it == m_tasks.end()) {
        return false;
    }
    return it->second->GetInfo().state == TaskState::Completed;
}

std::optional<TaskInfo> TaskSystem::GetTaskInfo(TaskId id) const {
    std::shared_lock<std::shared_mutex> lock(m_tasksMutex);
    auto it = m_tasks.find(id.Get());
    if (it == m_tasks.end()) {
        return std::nullopt;
    }
    return it->second->GetInfo();
}

std::vector<TaskInfo> TaskSystem::GetAllTaskInfo() const {
    std::vector<TaskInfo> result;
    std::shared_lock<std::shared_mutex> lock(m_tasksMutex);

    result.reserve(m_tasks.size());
    for (const auto& [id, task] : m_tasks) {
        result.push_back(task->GetInfo());
    }

    return result;
}

TaskSystem::Statistics TaskSystem::GetStatistics() const {
    std::shared_lock<std::shared_mutex> lock(m_tasksMutex);

    Statistics stats;
    stats.totalTasksProcessed = m_totalTasksProcessed;
    stats.failedTasks = m_failedTasks;
    stats.pendingTasks = m_tasks.size();

    if (m_totalTasksProcessed > 0) {
        stats.averageExecutionTime = m_totalExecutionTime / m_totalTasksProcessed;
        stats.averageWaitTime = m_totalWaitTime / m_totalTasksProcessed;
    }
    else {
        stats.averageExecutionTime = 0.0;
        stats.averageWaitTime = 0.0;
    }

    return stats;
}