#pragma once
#include "LockFreeStructures.h"

// 작업 우선순위
enum class TaskPriority {
    High,
    Normal,
    Low
};

// 작업 상태
enum class TaskState {
    Created,
    Scheduled,
    Running,
    Completed,
    Failed
};

// 작업 정보를 담는 구조체
struct TaskInfo {
    std::string name;
    TaskPriority priority;
    TaskState state;
    std::thread::id threadId;
    uint64_t startTime;
    uint64_t endTime;
    std::string error;
};

// 작업 식별자
class TaskId {
public:
    explicit TaskId(uint64_t id) : m_id(id) {}
    uint64_t Get() const { return m_id; }
private:
    uint64_t m_id;
};

// 작업 의존성 그래프를 위한 작업 노드
class TaskNode {
public:
    template<typename F>
    TaskNode(std::string name, TaskPriority priority, F&& func);

    TaskId GetId() const { return m_id; }
    const TaskInfo& GetInfo() const { return m_info; }
    void AddDependency(TaskId id) { m_dependencies.push_back(id); }

    bool IsReady() const;
    void Execute();

private:
    static std::atomic<uint64_t> s_nextId;
    TaskId m_id;
    TaskInfo m_info;
    std::function<void()> m_function;
    std::vector<TaskId> m_dependencies;
    std::atomic<size_t> m_remainingDependencies;

	friend class TaskSystem;
};

// 작업 시스템 클래스
class TaskSystem {
public:
    static TaskSystem& GetInstance();

    bool Initialize(size_t threadCount = std::thread::hardware_concurrency());
    void Shutdown();

    // 작업 생성 및 예약
    template<typename F>
    TaskId CreateTask(std::string name, TaskPriority priority, F&& func);

    template<typename F>
    TaskId CreateTaskWithDependencies(std::string name, TaskPriority priority, 
        F&& func, const std::vector<TaskId>& dependencies);

    // 작업 제어
    void WaitForTask(TaskId id);
    void WaitForAll();
    bool IsTaskComplete(TaskId id) const;

    // 작업 정보 조회
    std::optional<TaskInfo> GetTaskInfo(TaskId id) const;
    std::vector<TaskInfo> GetAllTaskInfo() const;

    // 성능 통계
    struct Statistics {
        size_t totalTasksProcessed;
        size_t failedTasks;
        size_t pendingTasks;
        double averageWaitTime;
        double averageExecutionTime;
    };

    Statistics GetStatistics() const;

private:
    TaskSystem() = default;
    ~TaskSystem() = default;
    TaskSystem(const TaskSystem&) = delete;
    TaskSystem& operator=(const TaskSystem&) = delete;

    void WorkerThread();
    void ScheduleTask(std::shared_ptr<TaskNode> task);
    void ProcessCompletedTask(const std::shared_ptr<TaskNode>& task);
    void UpdateDependencies(TaskId completedTaskId);

    static constexpr size_t HIGH_PRIORITY_QUEUE_SIZE = 1024;
    static constexpr size_t NORMAL_PRIORITY_QUEUE_SIZE = 4096;
    static constexpr size_t LOW_PRIORITY_QUEUE_SIZE = 4096;

    std::atomic<bool> m_isRunning{ false };
    std::vector<std::thread> m_workerThreads;

    // 우선순위별 작업 큐
    LockFreeRingBuffer<std::shared_ptr<TaskNode>, HIGH_PRIORITY_QUEUE_SIZE> m_highPriorityQueue;
    LockFreeRingBuffer<std::shared_ptr<TaskNode>, NORMAL_PRIORITY_QUEUE_SIZE> m_normalPriorityQueue;
    LockFreeRingBuffer<std::shared_ptr<TaskNode>, LOW_PRIORITY_QUEUE_SIZE> m_lowPriorityQueue;

    // 작업 추적을 위한 컨테이너
    mutable std::shared_mutex m_tasksMutex;
    std::unordered_map<uint64_t, std::shared_ptr<TaskNode>> m_tasks;

    // 통계
    std::atomic<size_t> m_totalTasksProcessed{ 0 };
    std::atomic<size_t> m_failedTasks{ 0 };
    std::atomic<double> m_totalWaitTime{ 0.0 };
    std::atomic<double> m_totalExecutionTime{ 0.0 };
};

// 템플릿 메서드 구현
template<typename F>
TaskNode::TaskNode(std::string name, TaskPriority priority, F&& func)
    : m_id(s_nextId.fetch_add(1))
    , m_function(std::forward<F>(func))
    , m_remainingDependencies(0)
{
    m_info.name = std::move(name);
    m_info.priority = priority;
    m_info.state = TaskState::Created;
    m_info.startTime = 0;
    m_info.endTime = 0;
}

template<typename F>
TaskId TaskSystem::CreateTask(std::string name, TaskPriority priority, F&& func) {
    auto task = std::make_shared<TaskNode>(std::move(name), priority, std::forward<F>(func));
    ScheduleTask(task);
    return task->GetId();
}

template<typename F>
TaskId TaskSystem::CreateTaskWithDependencies(std::string name, TaskPriority priority,
    F&& func, const std::vector<TaskId>& dependencies) {
    auto task = std::make_shared<TaskNode>(std::move(name), priority, std::forward<F>(func));

    {
        std::shared_lock<std::shared_mutex> lock(m_tasksMutex);
        size_t remainingDeps = 0;

        for (const auto& depId : dependencies) {
            auto it = m_tasks.find(depId.Get());
            if (it != m_tasks.end() &&
                it->second->GetInfo().state != TaskState::Completed) {
                task->AddDependency(depId);
                remainingDeps++;
            }
        }

        task->m_remainingDependencies = remainingDeps;
    }

    if (task->m_remainingDependencies == 0) {
        ScheduleTask(task);
    }

    return task->GetId();
}