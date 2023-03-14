#include <Events/EventBus.h>

#include <Logging/Log.h>

#include <memory>

namespace IneptEngine::Events {

    std::unique_ptr<Subscription> EventBus::Subscribe(EventType type, EventHandler handler)
    {
        std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
        auto subscription = std::make_unique<Subscription>(type, EventCategory::None, handler);
        m_subscriptions.emplace_back(std::move(subscription));
        LOG_TRACE("Subscribed!");
        return subscription;
    }

    std::unique_ptr<Subscription> EventBus::Subscribe(EventCategory category, EventHandler handler)
    {
        std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
        auto subscription = std::make_unique<Subscription>(EventType::None, category, handler);
        m_subscriptions.emplace_back(std::move(subscription));
        return subscription;
    }

    bool EventBus::IsSubscribed(EventType type, EventHandler handler)
    {
        std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
        for (const auto& subscription : m_subscriptions) {
            if (subscription->type == type && subscription->handler.target<EventHandler>() == handler.target<EventHandler>()) {
                return true;
            }
        }
        return false;
    }

    void EventBus::Unsubscribe(const Subscription& subscription)
    {
        std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
        auto it = std::find_if(m_subscriptions.begin(), m_subscriptions.end(), [&subscription](const auto& s) {
            return *s == subscription;
            });
        if (it != m_subscriptions.end()) {
            m_subscriptions.erase(it);
        }
    }

    void EventBus::Publish(std::unique_ptr<Event> event)
    {
        std::lock_guard<std::mutex> lock(m_eventsMutex);
        m_events.emplace_back(std::move(event));
    }

    void EventBus::PublishNow(std::unique_ptr<Event> event)
    {
        std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
        for (const auto& subscription : m_subscriptions) {
            if ((subscription->type == EventType::None) && ((subscription->category & event->GetCategory()) != EventCategory::None) ||
                (subscription->type == event->GetType()) && ((subscription->category & event->GetCategory()) == EventCategory::None))
            {
                subscription->handler(event.get());
            }
        }
    }

    void EventBus::ProcessEvents()
    {
        std::vector<std::unique_ptr<Event>> events;
        {
            std::lock_guard<std::mutex> lock(m_eventsMutex);
            std::lock_guard<std::mutex> lock2(m_subscriptionsMutex);
            if (m_events.empty() || m_subscriptions.empty()) {
                return;
            }
            events = std::move(m_events);
        }
        for (const auto& event : events) {
            for (const auto& subscription : m_subscriptions) {
                if ((subscription->type == EventType::None) && ((subscription->category & event->GetCategory()) != EventCategory::None) ||
                    (subscription->type == event->GetType()) && ((subscription->category & event->GetCategory()) == EventCategory::None))
                {
                    subscription->handler(event.get());
                }
            }
        }
    }
} //namespace IneptEngine::Events