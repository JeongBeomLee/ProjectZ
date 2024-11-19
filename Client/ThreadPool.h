#pragma once

// 스레드 작업 통계
struct ThreadStats {
	std::atomic<size_t> completedJobs{ 0 };         // 성공한 작업 수  
	std::atomic<size_t> failedJobs{ 0 };            // 실패한 작업 수  
	std::atomic<uint64_t> totalExecutionTime{ 0 };  // 스레드가 실행한 작업의 총 실행 시간  

	// 복사 생성자와 복사 할당 연산자를 삭제
	ThreadStats(const ThreadStats&) = delete;
	ThreadStats& operator=(const ThreadStats&) = delete;

	// 기본 생성자와 이동 생성자, 이동 할당 연산자를 명시적으로 선언
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

	// 작업 큐잉을 위한 템플릿 메서드
	// std::future<std::invoke_result<F>::type>으로 타입을 반환한다고 해도
	// 컴파일러에게 명시적으로 알려주지 않으면 값으로 해석한다. 따라서 typename을 사용한다.
	template<class F>
	auto EnqueueJob(F&& f, const char* debugName = nullptr) -> std::future<typename std::invoke_result<F>::type>;

	size_t GetPendingJobCount() const;	// 대기 중인 작업 수
	size_t GetThreadCount() const { return m_workers.size(); }	// 생성된 스레드 수
	const ThreadStats& GetThreadStats(std::thread::id threadId); // 스레드 통계
	std::vector<std::thread::id> GetThreadIds() const; // 스레드 ID 목록

	void DumpStats() const;	// 모든 스레드 통계 출력

	void Shutdown(); // 스레드 풀 종료

private:
	void WorkerThread(); // 스레드 함수
	void LogThreadError(const std::string& errorMsg); // 에러 로깅

	std::vector<std::thread> m_workers; // 스레드 벡터
	std::queue<std::function<void()>> m_jobs; // 작업 큐

	// 동기화 객체들(큐 뮤텍스, 조건 변수)
	mutable std::mutex m_queueMutex;
	std::condition_variable m_condition;
	bool m_stop; // 컨트롤 플래그

	// 디버깅을 위한 멤버들(통계 뮤텍스, 스레드 통계, 총 작업 수)
	mutable std::mutex m_statsMutex;
	std::unordered_map<std::thread::id, ThreadStats> m_threadStats;
	std::atomic<size_t> m_totalJobsProcessed{ 0 };

#ifdef _DEBUG
	struct JobDebugInfo {
		std::string name;
		std::chrono::steady_clock::time_point startTime;
	};
	// 각 스레드에서 실행 중인 작업의 이름과 시작 시간
	std::unordered_map<std::thread::id, JobDebugInfo> m_currentJobs;
#endif
};

template<class F>
auto ThreadPool::EnqueueJob(F&& f, const char* debugName) -> std::future<typename std::invoke_result<F>::type>
{
	// `F`가 callable이고, 호출했을 때 반환 타입이 무엇인지 계산하여 return_type에 저장.
	using return_type = typename std::invoke_result<F>::type;

	// `std::packaged_task`를 사용하여 작업을 캡슐화.
	// 작업이 완료되면 결과를 `std::future`로 반환할 수 있음.
	auto task = std::make_shared<std::packaged_task<return_type()>>(
		// 람다로 실제 작업을 정의. 이 작업은 나중에 스레드가 실행함.
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
					// `return_type`이 void인 경우 작업을 실행하고 바로 반환.
					f();
					return;
				}
				else {
					// `return_type`이 void가 아닌 경우 작업을 실행하고 결과를 반환.
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

	// `std::packaged_task`로부터 future를 가져옴. 호출자는 이 future를 통해 결과를 나중에 가져올 수 있음.
	std::future<return_type> res = task->get_future();

	{
		// 작업 큐에 새로운 작업을 추가. 작업 큐 접근은 뮤텍스를 사용해 보호.
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_jobs.emplace([task]() { (*task)(); }); // 큐에 람다 형태로 작업 추가.
	}

	// 작업을 대기 중인 스레드 중 하나를 깨움.
	m_condition.notify_one();

	// 작업 결과를 가져올 수 있는 future를 반환.
	return res;
}