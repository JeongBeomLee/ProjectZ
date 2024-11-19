#pragma once

// ������ �۾� ���
struct ThreadStats {
	std::atomic<size_t> completedJobs{ 0 };         // ������ �۾� ��  
	std::atomic<size_t> failedJobs{ 0 };            // ������ �۾� ��  
	std::atomic<uint64_t> totalExecutionTime{ 0 };  // �����尡 ������ �۾��� �� ���� �ð�  

	// ���� �����ڿ� ���� �Ҵ� �����ڸ� ����
	ThreadStats(const ThreadStats&) = delete;
	ThreadStats& operator=(const ThreadStats&) = delete;

	// �⺻ �����ڿ� �̵� ������, �̵� �Ҵ� �����ڸ� ��������� ����
	ThreadStats() = default;
	ThreadStats(ThreadStats&&) = default;
    ThreadStats& operator=(ThreadStats&&) noexcept {
		completedJobs = 0;
		failedJobs = 0;
		totalExecutionTime = 0;
		return *this;
	}
};

class ThreadPool {
public:
	ThreadPool(size_t threadCount = std::thread::hardware_concurrency());
	~ThreadPool();

	// �۾� ť���� ���� ���ø� �޼���
	// std::future<std::invoke_result<F>::type>���� Ÿ���� ��ȯ�Ѵٰ� �ص�
	// �����Ϸ����� ��������� �˷����� ������ ������ �ؼ��Ѵ�. ���� typename�� ����Ѵ�.
	template<class F>
	auto EnqueueJob(F&& f, const char* debugName = nullptr) -> std::future<typename std::invoke_result<F>::type>;

	size_t GetPendingJobCount() const;	// ��� ���� �۾� ��
	size_t GetThreadCount() const { return m_workers.size(); }	// ������ ������ ��
	const ThreadStats& GetThreadStats(std::thread::id threadId); // ������ ���
	std::vector<std::thread::id> GetThreadIds() const; // ������ ID ���

	void DumpStats() const;	// ��� ������ ��� ���

	void Shutdown(); // ������ Ǯ ����

private:
	void WorkerThread(); // ������ �Լ�
	void LogThreadError(const std::string& errorMsg); // ���� �α�

	std::vector<std::thread> m_workers; // ������ ����
	std::queue<std::function<void()>> m_jobs; // �۾� ť

	// ����ȭ ��ü��(ť ���ؽ�, ���� ����)
	mutable std::mutex m_queueMutex;
	std::condition_variable m_condition;
	bool m_stop; // ��Ʈ�� �÷���

	// ������� ���� �����(��� ���ؽ�, ������ ���, �� �۾� ��)
	mutable std::mutex m_statsMutex;
	std::unordered_map<std::thread::id, ThreadStats> m_threadStats;
	std::atomic<size_t> m_totalJobsProcessed{ 0 };

#ifdef _DEBUG
	struct JobDebugInfo {
		std::string name;
		std::chrono::steady_clock::time_point startTime;
	};
	// �� �����忡�� ���� ���� �۾��� �̸��� ���� �ð�
	std::unordered_map<std::thread::id, JobDebugInfo> m_currentJobs;
#endif
};

template<class F>
auto ThreadPool::EnqueueJob(F&& f, const char* debugName) -> std::future<typename std::invoke_result<F>::type>
{
	// `F`�� callable�̰�, ȣ������ �� ��ȯ Ÿ���� �������� ����Ͽ� return_type�� ����.
	using return_type = typename std::invoke_result<F>::type;

	// `std::packaged_task`�� ����Ͽ� �۾��� ĸ��ȭ.
	// �۾��� �Ϸ�Ǹ� ����� `std::future`�� ��ȯ�� �� ����.
	auto task = std::make_shared<std::packaged_task<return_type()>>(
		// ���ٷ� ���� �۾��� ����. �� �۾��� ���߿� �����尡 ������.
		[this, f = std::forward<F>(f), debugName]() -> return_type {
			auto threadId = std::this_thread::get_id();
			auto& stats = m_threadStats[threadId];

			try {
#ifdef _DEBUG
				if (debugName) {
					std::lock_guard<std::mutex> lock(m_statsMutex);
					m_currentJobs[threadId] = { debugName, std::chrono::steady_clock::now() };
				}
#endif
				auto start = std::chrono::steady_clock::now();

				if constexpr (std::is_void_v<return_type>) {
					// `return_type`�� void�� ��� �۾��� �����ϰ� �ٷ� ��ȯ.
					f();
					return;
				}
				else {
					// `return_type`�� void�� �ƴ� ��� �۾��� �����ϰ� ����� ��ȯ.
					return f();
				}

				auto end = std::chrono::steady_clock::now();
				stats.totalExecutionTime += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
				stats.completedJobs++;
			}
			catch (const std::exception& e) {
				stats.failedJobs++;
				LogThreadError(std::string("Job failed: ") + e.what());
				throw;
			}
		}
	);

	// `std::packaged_task`�κ��� future�� ������. ȣ���ڴ� �� future�� ���� ����� ���߿� ������ �� ����.
	std::future<return_type> res = task->get_future();

	{
		// �۾� ť�� ���ο� �۾��� �߰�. �۾� ť ������ ���ؽ��� ����� ��ȣ.
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_jobs.emplace([task]() { (*task)(); }); // ť�� ���� ���·� �۾� �߰�.
	}

	// �۾��� ��� ���� ������ �� �ϳ��� ����.
	m_condition.notify_one();

	// �۾� ����� ������ �� �ִ� future�� ��ȯ.
	return res;
}