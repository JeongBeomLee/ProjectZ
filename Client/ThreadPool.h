#pragma once
#include <queue>
#include <thread>
#include <vector>
#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>
#include <type_traits> // �߰��� include

class ThreadPool {
public:
    ThreadPool(size_t threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();

    // �۾� ť���� ���� ���ø� �޼���
    template<class F>
    auto EnqueueJob(F&& f) -> std::future<typename std::invoke_result<F>::type>;

    // ������ Ǯ ����
    void Shutdown();

private:
    // �۾��� ������ �Լ�
    void WorkerThread();

    // ������ �����̳�
    std::vector<std::thread> m_workers;

    // �۾� ť
    std::queue<std::function<void()>> m_jobs;

    // ����ȭ ��ü��
    std::mutex m_queueMutex;
    std::condition_variable m_condition;

    // ��Ʈ�� �÷���
    bool m_stop;
};

// ���ø� ����
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