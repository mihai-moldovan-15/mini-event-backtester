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
    const Timestamp m_sendLatency{ 3'000'000 };
    const Timestamp m_returnLatency{ 3'000'000 };
    const Timestamp m_timeDelta{ 3'000'000 };
    Timestamp m_endTime{ 21'000'000'000 };
    Portfolio m_portfolio;
    std::unordered_map<Symbol, OrderBook> m_orderBooks{};
    std::priority_queue<std::shared_ptr<Event>, std::vector<std::shared_ptr<Event>>, eventCompare> m_personalEvents{};
    std::vector<std::unique_ptr<Event>> m_historicalEvents{};
    std::unordered_map<OrderId, Symbol> m_OrderIdToSymbol; /// pentru CancelOrderEvent si ModifyOrderEvent
    std::vector<Fill> m_fillsRecord{};
    std::vector<ResponseEvent> m_pendingResponses{};


    std::unique_ptr<Strategy> m_strategy{};
    Timestamp m_strategyNextAvailableTime{};

    SizeValue m_historicalRows{};
    Cash m_minEquity{};
    Cash m_maxEquity{};
    std::unordered_map<Symbol, Position> m_finalPositions{};///pozitiile dinainte de lichidare
    void sampleEquity();

public:
    void run();

    ///ordinea din lista de initializare urmeaza ordinea de declarare a membrilor
    Simulator(std::unique_ptr<Strategy> strategy, Cash initialCash, Timestamp endTime = 1'000'000'000,
              Price commissionPerShare = 0) :
                            m_endTime(endTime), m_portfolio{initialCash, commissionPerShare},
                            m_strategy(std::move(strategy)) {}
    Timestamp getCurrTimeStamp() const { return m_currentTime; }
    const Portfolio& getPortfolio() const { return m_portfolio; }

    OrderBook& getOrderBook(const Symbol& symbol); //not ideal
    const OrderBook& getOrderBook(const Symbol& symbol) const;
    const std::unordered_map<Symbol, OrderBook>& getOrderBooks() const { return m_orderBooks; }
    OrderBook& getOrderBookForOrder(OrderId id);   //modify si cancel
    const Timestamp getReturnLatency() const { return m_returnLatency; }

    const std::vector<Fill>& getFillsRecord() const { return m_fillsRecord; }

    SizeValue getHistoricalRowCount() const { return m_historicalRows; }
    Cash getMinEquity() const { return m_minEquity; }
    Cash getMaxEquity() const { return m_maxEquity; }
    ///pozitia de dinainte de lichidarea finala; dupa run() cea din portofoliu e mereu 0
    Position getFinalPosition(const Symbol& symbol) const;

    void loadHistoricalEvents(const std::filesystem::path& dataFile);
    void scheduleTimer();
    void handleResponse(const ResponseEvent& resp);
    void runStrategy();

    void addBook(const Symbol& symbol);
    void cancelAllOpenOrders();

    void scheduleEvent(std::shared_ptr<Event> event) { m_personalEvents.push(std::move(event)); }
};