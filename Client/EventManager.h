#pragma once
#include "EventTypes.h"

class EventManager {
public:
    static EventManager& Instance() {
        static EventManager instance;
        return instance;
    }

    // 이벤트 발행 (어떤 스레드에서든 호출 가능)
    template<typename EventType>
    void Publish(const EventType& event) {
        //std::lock_guard<std::mutex> lock(m_mutex);
        GetQueue<EventType>().Push(event);
        Logger::Instance().Debug("이벤트 발행: {}", typeid(EventType).name());
    }

    // 이벤트 구독
    template<typename EventType>
    Event::EventDispatcher<EventType>::HandlerId Subscribe(
		Event::EventCallback<EventType> callback) {
        return GetDispatcher<EventType>().Subscribe(std::move(callback));
    }

    // 구독 취소
    template<typename EventType>
    void Unsubscribe(typename Event::EventDispatcher<EventType>::HandlerId id) {
        GetDispatcher<EventType>().Unsubscribe(id);
    }

    // 이벤트 처리 (메인 스레드에서만 호출)
    void Update() {
        ProcessEvents<Event::CollisionEvent>();
        ProcessEvents<Event::ResourceEvent>();
        ProcessEvents<Event::InputEvent>();
    }

private:
    EventManager() = default;
    ~EventManager() = default;

    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    template<typename EventType>
    Event::EventQueue<EventType>& GetQueue() {
        return std::get<Event::EventQueue<EventType>>(m_queues);
    }

    template<typename EventType>
    Event::EventDispatcher<EventType>& GetDispatcher() {
        return std::get<Event::EventDispatcher<EventType>>(m_dispatchers);
    }

    template<typename EventType>
    void ProcessEvents() {
        auto& queue = GetQueue<EventType>();
        auto& dispatcher = GetDispatcher<EventType>();

        EventType event;
        while (queue.Pop(event)) {
            dispatcher.Dispatch(event);
        }
    }

    // 모든 이벤트 타입에 대한 큐와 디스패처를 저장
    std::tuple<
        Event::EventQueue<Event::CollisionEvent>,
        Event::EventQueue<Event::ResourceEvent>,
        Event::EventQueue<Event::InputEvent>
    > m_queues;

    std::tuple<
        Event::EventDispatcher<Event::CollisionEvent>,
        Event::EventDispatcher<Event::ResourceEvent>,
        Event::EventDispatcher<Event::InputEvent>
    > m_dispatchers;

    //mutable std::mutex m_mutex;
};