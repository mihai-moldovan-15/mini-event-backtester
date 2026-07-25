#pragma once
#include "Types.hpp"
#include "Order.hpp"
#include <memory>
#include <optional>

class Simulator;
class Event {
private:
    Timestamp m_sentTs{};
    SequenceNumber m_sqNum{};
    inline static SequenceNumber m_nextSqNum{};
public:
    Event(Timestamp sentTs, SequenceNumber sqNum = ++m_nextSqNum): m_sentTs(sentTs), m_sqNum(sqNum) {}
    Event(const Event&) = delete; ///Event e abstract class, can t instantiate an Event object
    Event& operator=(const Event&) = delete;///same logic
    Timestamp getTimeStamp() const { return m_sentTs; }
    SequenceNumber getSequenceNumber() const { return m_sqNum; }///tot timpul voi procesa evenimentele istorice inaintea celor
                                                                ///celor personale
    void addLatency(Timestamp latency) { m_sentTs += latency; }

    virtual void execute(Simulator&) = 0;
    virtual ~Event() = default;
};

class TimerEvent: public Event {
public:
    explicit TimerEvent(Timestamp sentTs): Event(sentTs) {}
    void execute(Simulator& simulator) override;
};

class AddOrderEvent: public Event{
private:
    Order m_order;
public:
    AddOrderEvent(Timestamp sentTs, Order order): Event(sentTs), m_order(std::move(order)) {}
    void execute(Simulator& simulator) override;
};

class CancelOrderEvent: public Event {
private:
    OrderId m_orderId{};
public:
    CancelOrderEvent(Timestamp sentTs, OrderId orderId): Event(sentTs), m_orderId(orderId) {}
    void execute(Simulator& simulator) override;
};

class ModifyOrderEvent: public Event {
private:
    OrderId m_orderId{};
    Quantity m_newQuantity{};
    std::optional<Price> m_newLimitPrice{};
public:
    ModifyOrderEvent(Timestamp sentTs, OrderId id, Quantity q, std::optional<Price> p)
        : Event(sentTs), m_orderId(id), m_newQuantity(q), m_newLimitPrice(p) {}
    void execute(Simulator& s) override;
};

struct eventCompare {
    bool operator()(const std::unique_ptr<Event>& a,
                    const std::unique_ptr<Event>& b) const {
        if (a->getTimeStamp() != b->getTimeStamp())
            return a->getTimeStamp() > b->getTimeStamp();

        return a->getSequenceNumber() > b->getSequenceNumber();
    }
};