#include "Simulator.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <memory>
#include <chrono>
#include "Types.hpp"
#include "Event.hpp"

void Simulator::run() {
    size_t histIdx = 0;

    while ((histIdx < m_historicalEvents.size() || !m_personalEvents.empty()) && m_currentTime < m_endTime) {
        bool histAvailable = histIdx < m_historicalEvents.size();
        bool persAvailable = !m_personalEvents.empty();

        Timestamp histTs = histAvailable ? m_historicalEvents[histIdx]->getTimeStamp() : std::numeric_limits<Timestamp>::max();
        Timestamp persTs = persAvailable ? m_personalEvents.top()->getTimeStamp() : std::numeric_limits<Timestamp>::max();

        if (histAvailable && histTs <= persTs) {
            m_currentTime = histTs;
            m_historicalEvents[histIdx]->execute(*this);
            ++histIdx;
        }
        else {
            m_currentTime = persTs;
            auto event = std::move(const_cast<std::unique_ptr<Event>&>(m_personalEvents.top()));
            m_personalEvents.pop();
            event->execute(*this);
        }
    }
}


void Simulator::handleResponse(const ResponseEvent& resp) {
    if (!resp.isOwn)
        return;

    if (resp.type == ResponseType::Accepted || resp.type == ResponseType::PartiallyFilled)
        m_OrderIdToSymbol[resp.orderId] = resp.symbol;
    else if (resp.type == ResponseType::Filled || resp.type == ResponseType::Cancelled)
        m_OrderIdToSymbol.erase(resp.orderId);

    if (resp.type == ResponseType::Filled || resp.type == ResponseType::PartiallyFilled) {
        Fill fill{resp.symbol, resp.ts, resp.orderId, resp.side, *resp.fillQty, *resp.fillPrice};
        m_portfolio.applyFill(fill);
        m_fillsRecord.push_back(fill);
    }

    m_pendingResponses.push_back(resp);
}

void Simulator::loadHistoricalEvents(const std::filesystem::path& dataFile) {
    std::ifstream in(dataFile);

    if (!in)
        throw std::runtime_error("Could not open file " + dataFile.string());

    Order order;
    while (in >> order) {
        if (order.getQuantity() <= 0)
            continue;
        m_historicalEvents.push_back(std::make_unique<AddOrderEvent>(order.getTimeStamp(), order));
    }

    std::stable_sort(m_historicalEvents.begin(), m_historicalEvents.end(),
                [](const auto& a, const auto& b) { return a->getTimeStamp() < b->getTimeStamp(); });
}

OrderBook& Simulator::getOrderBookForOrder(OrderId id) {
    auto it = m_OrderIdToSymbol.find(id);
    if (it == m_OrderIdToSymbol.end())
        throw std::logic_error("Unknown order id");

    return m_orderBooks.at(it->second);
}

OrderBook& Simulator::getOrderBook(const Symbol& symbol) {
    auto it = m_orderBooks.find(symbol);

    if (it == m_orderBooks.end())
        throw std::logic_error("Unknown symbol " + symbol);

    return it->second;
}

const OrderBook& Simulator::getOrderBook(const Symbol& symbol) const {
    auto it = m_orderBooks.find(symbol);

    if (it == m_orderBooks.end())
        throw std::logic_error("Unknown symbol " + symbol);

    return it->second;
}

void Simulator::scheduleTimer() {
    Timestamp next = m_currentTime + m_timeDelta;
    if (next > m_endTime)
        return ;

    m_personalEvents.push(std::make_unique<TimerEvent>(next));
}

void Simulator::addBook(const Symbol& symbol) {
    if (m_orderBooks.find(symbol) == m_orderBooks.end())
        m_orderBooks.emplace(symbol, OrderBook(symbol));
    else
        throw std::logic_error("Book already exists for symbol: " + symbol);
}

void Simulator::runStrategy() {
    if (m_currentTime < m_strategyNextAvailableTime)
        return ;

    auto responses = std::move(m_pendingResponses);
    m_pendingResponses.clear();

    auto start = std::chrono::steady_clock::now();
    auto strategyActions = m_strategy->onTime(m_currentTime, m_orderBooks, m_portfolio, responses);
    auto end = std::chrono::steady_clock::now();

    Timestamp processingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    m_strategyNextAvailableTime = m_currentTime + processingTime;

    for (auto& action : strategyActions) {
        action->addLatency(m_latency);
        m_personalEvents.push(std::move(action));
    }
}