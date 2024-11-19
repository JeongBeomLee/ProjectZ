#include "pch.h"
#include "MemoryManager.h"
#include "Logger.h"

// MemoryPool ����
MemoryPool::MemoryPool(size_t blockSize, size_t totalSize)
    : m_blockSize(blockSize)
    , m_totalSize(totalSize)
    , m_usedMemory(0) {
    m_memory = new uint8_t[totalSize];
    InitializeFreeList();
}

MemoryPool::~MemoryPool() 
{
    delete[] m_memory;
}

void MemoryPool::InitializeFreeList() 
{
    m_freeList = reinterpret_cast<Block*>(m_memory);
    m_freeList->size = m_totalSize;
    m_freeList->next = nullptr;
}

void* MemoryPool::Allocate(size_t size, size_t alignment) 
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // ���� �䱸������ ����� ���� �ʿ� ũ�� ���
    size_t alignmentPadding = alignment - 1 + sizeof(Block);
    size_t totalSize = size + alignmentPadding;

    // ������ ��� ã��
    Block* block = FindFreeBlock(totalSize, alignment);
    if (!block) {
        CoalesceMemory();  // �޸� ���� ���� �õ�
        block = FindFreeBlock(totalSize, alignment);
        if (!block) {
            return nullptr;  // �Ҵ� ����
        }
    }

    // ��� ����
    size_t alignedAddress = (reinterpret_cast<size_t>(block) + sizeof(Block) + alignment - 1) & ~(alignment - 1);
    size_t headerAddress = alignedAddress - sizeof(Block);
    Block* alignedBlock = reinterpret_cast<Block*>(headerAddress);

    // ���� ������ ����ϴٸ� ���ο� free ��� ����
    size_t remainingSize = block->size - (alignedAddress - reinterpret_cast<size_t>(block));
    if (remainingSize > sizeof(Block)) {
        Block* newBlock = reinterpret_cast<Block*>(alignedAddress + size);
        newBlock->size = remainingSize - (alignedAddress - reinterpret_cast<size_t>(block));
        newBlock->next = block->next;
        m_freeList = newBlock;
    }

    alignedBlock->size = size;
    m_usedMemory += size;

    return reinterpret_cast<void*>(alignedAddress);
}

void MemoryPool::Deallocate(void* ptr) 
{
    if (!ptr || !OwnsPointer(ptr)) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    Block* block = reinterpret_cast<Block*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(Block));
    m_usedMemory -= block->size;

    // ������ ����� free ����Ʈ�� �߰�
    block->next = m_freeList;
    m_freeList = block;

    CoalesceMemory();  // ������ �� ��� ����
}

bool MemoryPool::OwnsPointer(void* ptr) const 
{
    return ptr >= m_memory && ptr < (m_memory + m_totalSize);
}

size_t MemoryPool::GetAvailableSpace() const 
{
    return m_totalSize - m_usedMemory;
}

MemoryPool::Block* MemoryPool::FindFreeBlock(size_t size, size_t alignment) 
{
    Block* prev = nullptr;
    Block* current = m_freeList;

    while (current) {
        size_t alignedAddress = (reinterpret_cast<size_t>(current) + sizeof(Block) + alignment - 1) & ~(alignment - 1);
        size_t headerAddress = alignedAddress - sizeof(Block);

        if (current->size >= size &&
            headerAddress >= reinterpret_cast<size_t>(current) &&
            headerAddress + size <= reinterpret_cast<size_t>(current) + current->size) {
            if (prev) {
                prev->next = current->next;
            }
            else {
                m_freeList = current->next;
            }
            return current;
        }

        prev = current;
        current = current->next;
    }

    return nullptr;
}

void MemoryPool::CoalesceMemory() 
{
    if (!m_freeList) {
        return;
    }

    Block* current = m_freeList;
    while (current->next) {
        if (reinterpret_cast<uint8_t*>(current) + current->size ==
            reinterpret_cast<uint8_t*>(current->next)) {
            current->size += current->next->size;
            current->next = current->next->next;
        }
        else {
            current = current->next;
        }
    }
}

// MemoryManager ����
MemoryManager& MemoryManager::GetInstance() 
{
    static MemoryManager instance;
    return instance;
}

bool MemoryManager::Initialize() 
{
    try {
        m_smallPool = std::make_unique<MemoryPool>(SMALL_BLOCK_SIZE, SMALL_POOL_SIZE);
        m_largePool = std::make_unique<MemoryPool>(SMALL_BLOCK_SIZE * 4, LARGE_POOL_SIZE);
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize MemoryManager: {}", e.what());
        return false;
    }
}

void MemoryManager::Shutdown() 
{
    DumpMemoryLeaks();
    m_smallPool.reset();
    m_largePool.reset();
}

void* MemoryManager::Allocate(size_t size, size_t alignment, const char* file, int line) 
{
    void* ptr = AllocateFromPool(size, alignment, file, line);
    if (!ptr) {
        LOG_ERROR("Memory allocation failed: size={}, alignment={}", size, alignment);
        throw std::bad_alloc();
    }
    return ptr;
}

void* MemoryManager::AllocateFromPool(size_t size, size_t alignment, const char* file, int line) 
{
    void* ptr = nullptr;

    // ���� ��� Ǯ���� ���� �õ�
    if (size <= SMALL_BLOCK_SIZE) {
        ptr = m_smallPool->Allocate(size, alignment);
    }

    // �����ϰų� ū ����̸� ū ��� Ǯ���� �õ�
    if (!ptr) {
        ptr = m_largePool->Allocate(size, alignment);
    }

    if (ptr) {
        TrackAllocation(ptr, size, alignment, file, line);
        m_totalAllocated += size;
    }

    return ptr;
}

void MemoryManager::Deallocate(void* ptr) 
{
    if (!ptr) return;

    UntrackAllocation(ptr);

    // �����Ͱ� ��� Ǯ�� ���ϴ��� Ȯ���ϰ� ����
    if (m_smallPool->OwnsPointer(ptr)) {
        m_smallPool->Deallocate(ptr);
    }
    else if (m_largePool->OwnsPointer(ptr)) {
        m_largePool->Deallocate(ptr);
    }
    else {
        LOG_WARNING("Attempting to free untracked memory at {}",
            static_cast<void*>(ptr));
    }
}

void MemoryManager::TrackAllocation(void* ptr, size_t size, size_t alignment, const char* file, int line) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_allocations[ptr] = {
        size,
        alignment,
        file,
        line,
        std::this_thread::get_id()
    };
}

void MemoryManager::UntrackAllocation(void* ptr) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_allocations.find(ptr);
    if (it != m_allocations.end()) {
        m_totalFreed += it->second.size;
        m_allocations.erase(it);
    }
}

size_t MemoryManager::GetTotalAllocated() const 
{
    return m_totalAllocated;
}

size_t MemoryManager::GetTotalFreed() const 
{
    return m_totalFreed;
}

size_t MemoryManager::GetCurrentUsage() const 
{
    return m_totalAllocated - m_totalFreed;
}

void MemoryManager::DumpMemoryLeaks() 
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_allocations.empty()) {
        LOG_INFO("No memory leaks detected");
        return;
    }

    LOG_WARNING("Memory leaks detected: {} allocations", m_allocations.size());

    for (const auto& [ptr, header] : m_allocations) {
        LOG_ERROR("Memory leak: {} bytes at {}",
            header.size,
            static_cast<void*>(const_cast<void*>(ptr)));

        if (header.file) {
            LOG_ERROR("    Allocated in {} at line {}", header.file, header.line);
        }

        LOG_ERROR("    Thread ID: {}",
            std::hash<std::thread::id>{}(header.threadId));
    }

    size_t totalLeaked = 0;
    for (const auto& alloc : m_allocations) {
        totalLeaked += alloc.second.size;
    }

    LOG_ERROR("Total leaked memory: {} bytes ({:.2f} MB)",
        totalLeaked,
        static_cast<float>(totalLeaked) / (1024 * 1024));
}

MemoryManager::~MemoryManager() 
{
    if (!m_allocations.empty()) {
        DumpMemoryLeaks();
    }
}