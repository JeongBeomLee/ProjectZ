#pragma once
#include <queue>
#include <thread>
#include <vector>
#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>
#include <type_traits> // 추가된 include

class ThreadPool {
public:
    ThreadPool(size_t threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();

    // 작업 큐잉을 위한 템플릿 메서드
    template<class F>
    auto EnqueueJob(F&& f) -> std::future<typename std::invoke_result<F>::type>;

    // 스레드 풀 종료
    void Shutdown();

private:
    // 작업자 스레드 함수
    void WorkerThread();

    // 스레드 컨테이너
    std::vector<std::thread> m_workers;

    // 작업 큐
    std::queue<std::function<void()>> m_jobs;

    // 동기화 객체들
    std::mutex m_queueMutex;
    std::condition_variable m_condition;

    // 컨트롤 플래그
    bool m_stop;
};

// 템플릿 구현
template<class F>
auto ThreadPool::EnqueueJob(F&& f) -> std::future<typename std::invoke_result<F>::type> {
    using return_type = typename std::invoke_result<F>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));
    std::future<return_type> res = task->get_future();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_jobs.emplace([task]() { (*task)(); });
    }

    m_condition.notify_one();
    return res;
}