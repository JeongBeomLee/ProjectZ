#pragma once
// �޸� ���� ���
constexpr size_t DEFAULT_ALIGNMENT = 16;
constexpr size_t CACHE_LINE_SIZE = 64;

// �޸� Ǯ�� �⺻ ũ��
constexpr size_t SMALL_BLOCK_SIZE = 256;
constexpr size_t SMALL_POOL_SIZE = 1024 * 1024;  // 1MB
constexpr size_t LARGE_POOL_SIZE = 16 * 1024 * 1024;  // 16MB

// �޸� �Ҵ� ������ �����ϱ� ���� ����ü
struct AllocationHeader {
    size_t size;                // ���� �Ҵ�� ũ��
    size_t alignment;           // ���� �䱸����
    const char* file;           // �Ҵ� ��û�� ����
    int line;                   // �Ҵ� ��û�� ����
    std::thread::id threadId;   // �Ҵ� ��û�� ������ ID
};

// �޸� Ǯ Ŭ����
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

// �޸� ������ Ŭ����
class MemoryManager {
public:
    static MemoryManager& GetInstance();

    // �ʱ�ȭ �� ����
    bool Initialize();
    void Shutdown();

    // �޸� �Ҵ�/���� �Լ�
    void* Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT,
        const char* file = nullptr, int line = 0);
    void Deallocate(void* ptr);

    // �޸� ��� ���
    size_t GetTotalAllocated() const;
    size_t GetTotalFreed() const;
    size_t GetCurrentUsage() const;
    void DumpMemoryLeaks();

    // STL �Ҵ��� ���
    template<typename T>
    class StlAllocator;

private:
    MemoryManager() = default;
    ~MemoryManager();
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    std::unique_ptr<MemoryPool> m_smallPool;  // ���� ��Ͽ� Ǯ
    std::unique_ptr<MemoryPool> m_largePool;  // ū ��Ͽ� Ǯ

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

// STL �Ҵ��� ����
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

// ��ũ�� ����
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