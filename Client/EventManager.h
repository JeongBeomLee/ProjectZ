#pragma once
#include "EventTypes.h"

class EventManager {
public:
    static EventManager& Instance() {
        static EventManager instance;
        return instance;
    }

    // �̺�Ʈ ���� (� �����忡���� ȣ�� ����)
    template<typename EventType>
    void Publish(const EventType& event) {
        //std::lock_guard<std::mutex> lock(m_mutex);
        GetQueue<EventType>().Push(event);
        Logger::Instance().Debug("�̺�Ʈ ����: {}", typeid(EventType).name());
    }

    // �̺�Ʈ ����
    template<typename EventType>
    Event::EventDispatcher<EventType>::HandlerId Subscribe(
		Event::EventCallback<EventType> callback) {
        return GetDispatcher<EventType>().Subscribe(std::move(callback));
    }

    // ���� ���
    template<typename EventType>
    void Unsubscribe(typename Event::EventDispatcher<EventType>::HandlerId id) {
        GetDispatcher<EventType>().Unsubscribe(id);
    }

    // �̺�Ʈ ó�� (���� �����忡���� ȣ��)
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

    // ��� �̺�Ʈ Ÿ�Կ� ���� ť�� ����ó�� ����
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