#pragma once

// 태그 포인터를 위한 유틸리티
template<typename T>
class TaggedPointer {
public:
    static constexpr uintptr_t TAG_MASK = (1ull << 16) - 1;
    static constexpr uintptr_t PTR_MASK = ~TAG_MASK;

    TaggedPointer() noexcept : m_value(0) {}

    TaggedPointer(T* ptr, uint16_t tag) noexcept {
        uintptr_t ptrValue = reinterpret_cast<uintptr_t>(ptr);
        m_value = (ptrValue & PTR_MASK) | (tag & TAG_MASK);
    }

    T* GetPointer() const noexcept {
        return reinterpret_cast<T*>(m_value & PTR_MASK);
    }

    uint16_t GetTag() const noexcept {
        return static_cast<uint16_t>(m_value & TAG_MASK);
    }

    bool operator==(const TaggedPointer& other) const noexcept {
        return m_value == other.m_value;
    }

private:
    uintptr_t m_value;
};

// 락프리 큐의 노드
template<typename T>
struct QueueNode {
    T data;
    std::atomic<QueueNode*> next;

    template<typename... Args>
    explicit QueueNode(Args&&... args)
        : data(std::forward<Args>(args)...)
        , next(nullptr)
    {}
};

// 락프리 큐
template<typename T>
class LockFreeQueue {
public:
    LockFreeQueue() {
        auto dummy = new QueueNode<T>{};
        m_head = dummy;
        m_tail = dummy;
    }

    ~LockFreeQueue() {
        T dummy;
        while (TryDequeue(dummy)) {}
        delete m_head.load();
    }

    // 복사/이동 금지
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
    LockFreeQueue(LockFreeQueue&&) = delete;
    LockFreeQueue& operator=(LockFreeQueue&&) = delete;

    void Enqueue(const T& value) {
        auto newNode = new QueueNode<T>(value);

        while (true) {
            auto tail = m_tail.load();
            auto next = tail->next.load();

            if (tail == m_tail.load()) {
                if (next == nullptr) {
                    if (tail->next.compare_exchange_weak(next, newNode)) {
                        m_tail.compare_exchange_weak(tail, newNode);
                        return;
                    }
                }
                else {
                    m_tail.compare_exchange_weak(tail, next);
                }
            }
        }
    }

    bool TryDequeue(T& result) {
        while (true) {
            auto head = m_head.load();
            auto tail = m_tail.load();
            auto next = head->next.load();

            if (head == m_head.load()) {
                if (head == tail) {
                    if (next == nullptr) {
                        return false;
                    }
                    m_tail.compare_exchange_weak(tail, next);
                }
                else {
                    result = next->data;
                    if (m_head.compare_exchange_weak(head, next)) {
                        delete head;
                        return true;
                    }
                }
            }
        }
    }

    bool IsEmpty() const {
        auto head = m_head.load();
        auto tail = m_tail.load();
        return (head == tail) && (head->next == nullptr);
    }

private:
    std::atomic<QueueNode<T>*> m_head;
    std::atomic<QueueNode<T>*> m_tail;
};

// 락프리 스택의 노드
template<typename T>
struct StackNode {
    T data;
    StackNode* next;

    template<typename... Args>
    explicit StackNode(Args&&... args)
        : data(std::forward<Args>(args)...)
        , next(nullptr)
    {}
};

// 락프리 스택
template<typename T>
class LockFreeStack {
private:
    struct TaggedNode {
        StackNode<T>* ptr;
        uint64_t tag;
    };

public:
    LockFreeStack() : m_head{ nullptr, 0 } {}
    ~LockFreeStack() {
        T dummy;
        while (TryPop(dummy)) {}
    }

    // 복사/이동 금지
    LockFreeStack(const LockFreeStack&) = delete;
    LockFreeStack& operator=(const LockFreeStack&) = delete;
    LockFreeStack(LockFreeStack&&) = delete;
    LockFreeStack& operator=(LockFreeStack&&) = delete;

    void Push(const T& value) {
        auto newNode = new StackNode<T>(value);
        TaggedNode oldHead = m_head.load();
        TaggedNode newHead;

        do {
            newNode->next = oldHead.ptr;
            newHead.ptr = newNode;
            newHead.tag = oldHead.tag + 1;
        } while (!m_head.compare_exchange_weak(oldHead, newHead));
    }

    bool TryPop(T& result) {
        TaggedNode oldHead = m_head.load();
        TaggedNode newHead;

        do {
            if (oldHead.ptr == nullptr) {
                return false;
            }

            newHead.ptr = oldHead.ptr->next;
            newHead.tag = oldHead.tag + 1;
            result = oldHead.ptr->data;
        } while (!m_head.compare_exchange_weak(oldHead, newHead));

        delete oldHead.ptr;
        return true;
    }

    bool IsEmpty() const {
        return m_head.load().ptr == nullptr;
    }

private:
    std::atomic<TaggedNode> m_head;
};

// 락프리 링 버퍼
template<typename T, size_t Size>
class LockFreeRingBuffer {
	static_assert(Size > 0 && (Size & (Size - 1)) == 0, "버퍼 크기는 2의 제곱수여야 합니다.");

public:
    LockFreeRingBuffer() : m_writeIndex(0), m_readIndex(0) {}

    bool TryEnqueue(const T& item) {
        auto currentWrite = m_writeIndex.load();
        auto currentRead = m_readIndex.load();

        if ((currentWrite + 1) % Size == currentRead) {
            return false;  // 버퍼가 가득 참
        }

        m_buffer[currentWrite] = item;
        m_writeIndex.store((currentWrite + 1) % Size);
        return true;
    }

    bool TryDequeue(T& item) {
        auto currentRead = m_readIndex.load();
        if (currentRead == m_writeIndex.load()) {
            return false;  // 버퍼가 비어있음
        }

        item = m_buffer[currentRead];
        m_readIndex.store((currentRead + 1) % Size);
        return true;
    }

    bool IsEmpty() const {
        return m_readIndex.load() == m_writeIndex.load();
    }

    bool IsFull() const {
        return (m_writeIndex.load() + 1) % Size == m_readIndex.load();
    }

private:
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> m_writeIndex;
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> m_readIndex;
    alignas(CACHE_LINE_SIZE) T m_buffer[Size];
};