#pragma once
#include "pch.h"
#include "Logger.h"

namespace Event
{
    class IEvent {
    public:
        virtual ~IEvent() = default;

        const std::chrono::system_clock::time_point& GetTimestamp() const {
            return m_timestamp;
        }

    protected:
        IEvent() : m_timestamp(std::chrono::system_clock::now()) {}

    private:
        std::chrono::system_clock::time_point m_timestamp;
    };

    template<typename EventType>
    class IEventHandler {
    public:
        virtual ~IEventHandler() = default;
        virtual void Handle(const EventType& event) = 0;
    };
    
    template<typename EventType>
    class EventCallback {
    public:
        using CallbackFn = std::function<void(const EventType&)>;

        EventCallback(CallbackFn callback)
            : m_callback(std::move(callback)) {}

        void operator()(const EventType& event) {
            m_callback(event);
        }

    private:
        CallbackFn m_callback;
    };

    // TBB concurrent_queue를 사용하는 이벤트 큐
    template<typename EventType>
    class EventQueue {
    public:
        void Push(const EventType& event) {
            m_events.push(event);
            Logger::Instance().Debug("이벤트가 큐에 추가됨: {}", typeid(EventType).name());
        }

        bool Pop(EventType& event) {
            return m_events.try_pop(event);
        }

        bool IsEmpty() const {
            return m_events.empty();
        }

        size_t Size() const {
            return m_events.size();
        }

    private:
        tbb::concurrent_queue<EventType> m_events;
    };

    // concurrent_unordered_map을 사용하는 디스패처
    template<typename EventType>
    class EventDispatcher {
    public:
        using HandlerId = size_t;

        HandlerId Subscribe(EventCallback<EventType> callback) {
            HandlerId id = m_nextHandlerId.fetch_add(1);
            m_handlers.insert({ id, std::move(callback) });
            Logger::Instance().Debug("이벤트 핸들러가 등록됨. ID: {}, 이벤트 타입: {}",
                id, typeid(EventType).name());
            return id;
        }

        void Unsubscribe(HandlerId id) {
            // erase는 thread-safe하지 않으므로 mutex 사용
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_handlers.unsafe_erase(id) > 0) {
                Logger::Instance().Debug("이벤트 핸들러가 제거됨. ID: {}", id);
            }
        }
        void Dispatch(const EventType& event) {
            // range-based for loop는 내부적으로 iterator를 사용하므로,
            // concurrent_unordered_map의 snapshot을 만들어서 사용
            auto handlers = m_handlers; // creates a snapshot
            for (auto& [id, handler] : handlers) {
                try {
                    handler(event);
                }
                catch (const std::exception& e) {
                    Logger::Instance().Error("이벤트 핸들러 실행 실패. ID: {}, 오류: {}",
                        id, e.what());
                }
            }
        }

    private:
        tbb::concurrent_unordered_map<HandlerId, EventCallback<EventType>> m_handlers;
        std::atomic<HandlerId> m_nextHandlerId{ 1 };
        std::mutex m_mutex; // Unsubscribe용 mutex
    };
}