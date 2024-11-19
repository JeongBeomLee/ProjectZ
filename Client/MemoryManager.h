#pragma once
// 메모리 정렬 상수
constexpr size_t DEFAULT_ALIGNMENT = 16;
constexpr size_t CACHE_LINE_SIZE = 64;

// 메모리 풀의 기본 크기
constexpr size_t SMALL_BLOCK_SIZE = 256;
constexpr size_t SMALL_POOL_SIZE = 1024 * 1024;  // 1MB
constexpr size_t LARGE_POOL_SIZE = 16 * 1024 * 1024;  // 16MB

// 메모리 할당 정보를 추적하기 위한 구조체
struct AllocationHeader {
    size_t size;                // 실제 할당된 크기
    size_t alignment;           // 정렬 요구사항
    const char* file;           // 할당 요청한 파일
    int line;                   // 할당 요청한 라인
    std::thread::id threadId;   // 할당 요청한 스레드 ID
};

// 메모리 풀 클래스
class MemoryPool {
public:
    MemoryPool(size_t blockSize, size_t totalSize);
    ~MemoryPool();

    void* Allocate(size_t size, size_t alignment);
    void Deallocate(void* ptr);
    bool OwnsPointer(void* ptr) const;
    size_t GetAvailableSpace() const;

private:
    struct Block {
        Block* next;
        size_t size;
    };

    std::mutex m_mutex;
    uint8_t* m_memory;
    Block* m_freeList;
    const size_t m_blockSize;
    const size_t m_totalSize;
    std::atomic<size_t> m_usedMemory;

    void InitializeFreeList();
    Block* FindFreeBlock(size_t size, size_t alignment);
    void CoalesceMemory();
};

// 메모리 관리자 클래스
class MemoryManager {
public:
    static MemoryManager& GetInstance();

    // 초기화 및 종료
    bool Initialize();
    void Shutdown();

    // 메모리 할당/해제 함수
    void* Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT,
        const char* file = nullptr, int line = 0);
    void Deallocate(void* ptr);

    // 메모리 사용 통계
    size_t GetTotalAllocated() const;
    size_t GetTotalFreed() const;
    size_t GetCurrentUsage() const;
    void DumpMemoryLeaks();

    // STL 할당자 얻기
    template<typename T>
    class StlAllocator;

private:
    MemoryManager() = default;
    ~MemoryManager();
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    std::unique_ptr<MemoryPool> m_smallPool;  // 작은 블록용 풀
    std::unique_ptr<MemoryPool> m_largePool;  // 큰 블록용 풀

    mutable std::mutex m_mutex;
    std::unordered_map<void*, AllocationHeader> m_allocations;

    std::atomic<size_t> m_totalAllocated{ 0 };
    std::atomic<size_t> m_totalFreed{ 0 };

    void* AllocateFromPool(size_t size, size_t alignment,
        const char* file, int line);
    void TrackAllocation(void* ptr, size_t size, size_t alignment,
        const char* file, int line);
    void UntrackAllocation(void* ptr);
};

// STL 할당자 구현
template<typename T>
class MemoryManager::StlAllocator {
public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    StlAllocator() noexcept = default;
    template<class U> StlAllocator(const StlAllocator<U>&) noexcept {}

    T* allocate(size_t n) {
        return static_cast<T*>(MemoryManager::GetInstance().Allocate(
            n * sizeof(T), alignof(T)));
    }

    void deallocate(T* p, size_t) {
        MemoryManager::GetInstance().Deallocate(p);
    }
};

// 매크로 정의
#ifdef _DEBUG
#define ALLOC(size, alignment) \
        MemoryManager::GetInstance().Allocate(size, alignment, __FILE__, __LINE__)
#define FREE(ptr) \
        MemoryManager::GetInstance().Deallocate(ptr)
#else
#define ALLOC(size, alignment) \
        MemoryManager::GetInstance().Allocate(size, alignment)
#define FREE(ptr) \
        MemoryManager::GetInstance().Deallocate(ptr)
#endif