#pragma once
#include "Types.hpp"
#include "Portfolio.hpp"
#include "Event.hpp"
#include "Strategy.hpp"
#include <unordered_map>
#include "Fill.hpp"
#include "OrderBook.hpp"
#include <queue>
#include <memory>
#include <filesystem>

class Simulator {
private:
    Timestamp m_currentTime{};
    const Timestamp m_latency{ 3'000'000 };
    const Timestamp m_timeDelta{ 2'000'000 };
    Timestamp m_endTime{ 1'000'000'000 };
    Portfolio m_portfolio;
    std::unordered_map<Symbol, OrderBook> m_orderBooks{};
    std::priority_queue<std::unique_ptr<Event>, std::vector<std::unique_ptr<Event>>, eventCompare> m_personalEvents{};
    std::vector<std::unique_ptr<Event>> m_historicalEvents{};
    std::unordered_map<OrderId, Symbol> m_OrderIdToSymbol; /// pentru CancelOrderEvent si ModifyOrderEvent
    std::vector<Fill> m_fillsRecord{};
    std::vector<ResponseEvent> m_pendingResponses{};

    std::unique_ptr<Strategy> m_strategy{};
    Timestamp m_strategyNextAvailableTime{};
public:
    void run();

    Simulator(Cash initialCash, Timestamp endTime = 1'000'000'000) : m_portfolio{initialCash}, m_endTime(endTime){}
    void setStrategy(std::unique_ptr<Strategy> strategy) { m_strategy = std::move(strategy); }
    Timestamp getCurrTimeStamp() const { return m_currentTime; }
    const Portfolio& getPortfolio() const { return m_portfolio; }

    OrderBook& getOrderBook(const Symbol& symbol); //not ideal
    const OrderBook& getOrderBook(const Symbol& symbol) const;
    const std::unordered_map<Symbol, OrderBook>& getOrderBooks() const { return m_orderBooks; }
    OrderBook& getOrderBookForOrder(OrderId id);///modify si cancel

    const std::vector<Fill>& getFillsRecord() const { return m_fillsRecord; }

    void loadHistoricalEvents(const std::filesystem::path& dataFile);
    void scheduleTimer();
    void addBook(const Symbol& symbol);
    void handleResponse(const ResponseEvent& resp);
    void runStrategy();


    void scheduleEvent(std::unique_ptr<Event> event) { m_personalEvents.push(std::move(event)); }///pentru testare
};