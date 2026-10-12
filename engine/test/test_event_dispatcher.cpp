// SPDX-License-Identifier: MIT
#include "ocf/core/event/EventDispatcher.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

using namespace ocf;

namespace {

class TestEvent : public Event {
public:
    TestEvent()
        : Event(EventType::Custom)
    {
    }
};

class OtherEvent : public Event {
public:
    OtherEvent()
        : Event(EventType::Custom)
    {
    }
};

} // namespace

TEST(EventDispatcherTest, CallsListenersOfTheEventType)
{
    EventDispatcher dispatcher;
    int testCount = 0;
    int otherCount = 0;
    Subscription test = dispatcher.subscribe<TestEvent>([&](TestEvent&) { testCount++; });
    Subscription other = dispatcher.subscribe<OtherEvent>([&](OtherEvent&) { otherCount++; });

    TestEvent event;
    dispatcher.dispatch(event);

    EXPECT_EQ(testCount, 1);
    EXPECT_EQ(otherCount, 0);
}

TEST(EventDispatcherTest, DestroyingSubscriptionUnsubscribes)
{
    EventDispatcher dispatcher;
    int count = 0;
    {
        Subscription subscription = dispatcher.subscribe<TestEvent>([&](TestEvent&) { count++; });
        EXPECT_TRUE(subscription.isActive());
    }

    TestEvent event;
    dispatcher.dispatch(event);

    EXPECT_EQ(count, 0);
}

TEST(EventDispatcherTest, ResetUnsubscribes)
{
    EventDispatcher dispatcher;
    int count = 0;
    Subscription subscription = dispatcher.subscribe<TestEvent>([&](TestEvent&) { count++; });

    subscription.reset();
    TestEvent event;
    dispatcher.dispatch(event);

    EXPECT_FALSE(subscription.isActive());
    EXPECT_EQ(count, 0);
}

TEST(EventDispatcherTest, MovedSubscriptionStaysSubscribed)
{
    EventDispatcher dispatcher;
    int count = 0;
    Subscription moved;
    {
        Subscription subscription = dispatcher.subscribe<TestEvent>([&](TestEvent&) { count++; });
        moved = std::move(subscription);
        EXPECT_FALSE(subscription.isActive());
    }

    TestEvent event;
    dispatcher.dispatch(event);

    EXPECT_TRUE(moved.isActive());
    EXPECT_EQ(count, 1);
}

TEST(EventDispatcherTest, HandledEventStopsPropagation)
{
    EventDispatcher dispatcher;
    int secondCount = 0;
    Subscription first =
        dispatcher.subscribe<TestEvent>([](TestEvent& event) { event.setHandled(true); });
    Subscription second = dispatcher.subscribe<TestEvent>([&](TestEvent&) { secondCount++; });

    TestEvent event;
    dispatcher.dispatch(event);

    EXPECT_EQ(secondCount, 0);
}

TEST(EventDispatcherTest, ListenerCanUnsubscribeItselfDuringDispatch)
{
    EventDispatcher dispatcher;
    int firstCount = 0;
    int secondCount = 0;
    Subscription first;
    first = dispatcher.subscribe<TestEvent>([&](TestEvent&) {
        firstCount++;
        first.reset();
    });
    Subscription second = dispatcher.subscribe<TestEvent>([&](TestEvent&) { secondCount++; });

    TestEvent event1;
    dispatcher.dispatch(event1);
    TestEvent event2;
    dispatcher.dispatch(event2);

    EXPECT_EQ(firstCount, 1);
    EXPECT_EQ(secondCount, 2);
}

TEST(EventDispatcherTest, ListenerAddedDuringDispatchIsCalledFromTheNextOne)
{
    EventDispatcher dispatcher;
    int addedCount = 0;
    std::vector<Subscription> added;
    Subscription adder = dispatcher.subscribe<TestEvent>([&](TestEvent&) {
        added.push_back(dispatcher.subscribe<TestEvent>([&](TestEvent&) { addedCount++; }));
    });

    TestEvent event1;
    dispatcher.dispatch(event1);
    EXPECT_EQ(addedCount, 0);

    TestEvent event2;
    dispatcher.dispatch(event2);
    EXPECT_EQ(addedCount, 1);
}

TEST(EventDispatcherTest, SubscriptionCanOutliveDispatcher)
{
    Subscription subscription;
    {
        EventDispatcher dispatcher;
        subscription = dispatcher.subscribe<TestEvent>([](TestEvent&) {});
    }

    EXPECT_FALSE(subscription.isActive());
    subscription.reset();
}
