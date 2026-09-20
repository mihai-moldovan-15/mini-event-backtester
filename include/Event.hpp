#pragma once
#include "Types.hpp"
#include "Order.hpp"
#include "ResponseEvent.hpp"
#include <memory>
#include <optional>
#include <utility>

class Simulator;
class Event {
private:
    Timestamp m_sentTs{};
    SequenceNumber m_sqNum{};
    // TODO: Contorul static poate cauza coliziuni de numere de secvență între multiple instanțe de Simulator sau la repornirea programului
    inline static SequenceNumber m_nextSqNum{};
public:
    Event(Timestamp sentTs, SequenceNumber sqNum = ++m_nextSqNum): m_sentTs(sentTs), m_sqNum(sqNum) {}
    Event(const Event&) = delete; ///Event e abstract class, can t instantiate an Event object
    Event& operator=(const Event&) = delete;///same logic
    [[nodiscard]] Timestamp getTimeStamp() const { return m_sentTs; }
    [[nodiscard]] SequenceNumber getSequenceNumber() const { return m_sqNum; }  ///tot timpul voi procesa evenimentele istorice inaintea celor
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

class AddPersonalOrderEvent: public Event{
private:
    Order m_order;
public:
    AddPersonalOrderEvent(Timestamp sentTs, Order order): Event(sentTs), m_order(std::move(order)) {}
    void execute(Simulator& simulator) override;
};

class CancelPersonalOrderEvent: public Event {
private:
    OrderId m_orderId{};
public:
    CancelPersonalOrderEvent(Timestamp sentTs, OrderId orderId): Event(sentTs), m_orderId(orderId) {}
    void execute(Simulator& simulator) override;
};

class CancelHistoricalOrderEvent: public Event {
private:
    OrderId m_orderId{};
    Symbol m_symbol{};
public:
    CancelHistoricalOrderEvent(Timestamp sentTs, OrderId orderId, Symbol symbol):
    Event(sentTs), m_orderId(orderId), m_symbol(std::move(symbol)) {}

    void execute(Simulator &) override;
};

class ModifyPersonalOrderEvent: public Event {
private:
    OrderId m_orderId{};
    Quantity m_newQuantity{};
    std::optional<Price> m_newLimitPrice{};
public:
    ModifyPersonalOrderEvent(Timestamp sentTs, OrderId id, Quantity q, std::optional<Price> p)
        : Event(sentTs), m_orderId(id), m_newQuantity(q), m_newLimitPrice(p) {}
    void execute(Simulator& s) override;
};

class ModifyHistoricalOrderEvent: public Event {
private:
    OrderId m_orderId{};
    Quantity m_newQuantity{};
    std::optional<Price> m_newLimitPrice{};
    Symbol m_symbol{};
public:
    ModifyHistoricalOrderEvent(Timestamp sentTs, OrderId id, Quantity q, std::optional<Price> p, Symbol symbol)
        : Event(sentTs), m_orderId(id), m_newQuantity(q), m_newLimitPrice(p), m_symbol(std::move(symbol)) {}
    void execute(Simulator& s) override;
};

class MarketReturnEvent : public Event {
private:
    ResponseEvent m_response{};
public:
    MarketReturnEvent(Timestamp ts, ResponseEvent response)
        : Event(ts), m_response(std::move(response)) {}

    void execute(Simulator& simulator) override;
};

struct eventCompare {
    bool operator()(const std::shared_ptr<Event>& a,
                    const std::shared_ptr<Event>& b) const {
        if (a->getTimeStamp() != b->getTimeStamp())
            return a->getTimeStamp() > b->getTimeStamp();

        return a->getSequenceNumber() > b->getSequenceNumber();
    }
};