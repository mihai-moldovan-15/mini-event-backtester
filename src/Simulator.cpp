#include "Simulator.hpp"

#include <algorithm>
#include <fstream>
#include <memory>
#include <chrono>
#include <sstream>
#include <stdexcept>
#include "Types.hpp"
#include "Event.hpp"

void Simulator::sampleEquity() {
    for (const auto& [symbol, book] : m_orderBooks)
        if (book.getBestBid() == 0 && book.getBestAsk() == 0)
            return;

    const Cash equity = m_portfolio.getEquity(m_orderBooks);
    m_minEquity = std::min(m_minEquity, equity);
    m_maxEquity = std::max(m_maxEquity, equity);
}

Position Simulator::getFinalPosition(const Symbol& symbol) const {
    auto it = m_finalPositions.find(symbol);
    return (it != m_finalPositions.end()) ? it->second : Position{};
}

void Simulator::run() {
    size_t histIdx = 0;

    m_minEquity = m_maxEquity = m_portfolio.getAvailableCash();

    while (histIdx < m_historicalEvents.size() || !m_personalEvents.empty()) {
        bool histAvailable = histIdx < m_historicalEvents.size();
        bool persAvailable = !m_personalEvents.empty();

        Timestamp histTs = histAvailable ? m_historicalEvents[histIdx]->getTimeStamp() : std::numeric_limits<Timestamp>::max();
        Timestamp persTs = persAvailable ? m_personalEvents.top()->getTimeStamp() : std::numeric_limits<Timestamp>::max();

        if (std::min(histTs, persTs) >= m_endTime)
            break;

        if (histAvailable && histTs <= persTs) {
            m_currentTime = histTs;
            m_historicalEvents[histIdx]->execute(*this);
            ++histIdx;
        }
        else {
            m_currentTime = persTs;
            // REVIEW: Folosirea const_cast pentru a muta din priority_queue este comportament nedefinit - top() returnează referință const
            // Raspuns: am pus shared_ptr in loc de unique_ptr
            std::shared_ptr<Event> event = m_personalEvents.top();
            m_personalEvents.pop();
            event->execute(*this);
        }
        sampleEquity();
    }

    for (const auto& [symbol, _] : m_orderBooks)
        m_finalPositions[symbol] = m_portfolio.getPosition(symbol);

    m_portfolio.liquidate(m_orderBooks);
    cancelAllOpenOrders();
}

void Simulator::cancelAllOpenOrders() {
    std::vector<OrderId> ids;
    ids.reserve(m_OrderIdToSymbol.size());
    for (const auto& [orderId, symbol] : m_OrderIdToSymbol)
        ids.push_back(orderId);

    for (OrderId id : ids) {
        auto it = m_OrderIdToSymbol.find(id);
        if (it == m_OrderIdToSymbol.end())
            continue;
        auto response = m_orderBooks.at(it->second).processCancelOrder(id, m_currentTime, true);
        handleResponse(response);
    }
}

void Simulator::handleResponse(const ResponseEvent& resp) {
    if (!resp.isOwn)
        return;

    if (resp.type == ResponseType::Resting || resp.type == ResponseType::PartiallyFilled)
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

    ///ts action orderId ownerId SIDE quantity price
    const Symbol symbol{"AAA"};
    constexpr Timestamp tsScale{10'000'000};///ts urile din fisier sunt tickuri, motorul lucreaza in ns: 1 tick = 10ms
    std::unordered_map<OrderId, Symbol> histSymbols;
    OrderId maxHistId{};

    std::string line;
    SizeValue lineNumber{};

    while (std::getline(in, line)) {
        ++lineNumber;
        if (line.empty())
            continue;

        std::istringstream stream(line);
        Timestamp ts{};
        std::string actionText, sideText;
        OrderId orderId{};
        OrderId ownerId{};
        Quantity quantity{};
        Price price{};

        const auto lineInfo = [&] { return " on line " + std::to_string(lineNumber) + " of " + dataFile.string(); };

        if (!(stream >> ts >> actionText >> orderId >> ownerId >> sideText >> quantity >> price))
            throw std::invalid_argument("Malformed event" + lineInfo());

        ts *= tsScale;

        std::ranges::transform(actionText, actionText.begin(), ::toupper);
        std::ranges::transform(sideText, sideText.begin(), ::toupper);

        Side side{};
        if (sideText == "BUY")
            side = Side::Buy;
        else if (sideText == "SELL")
            side = Side::Sell;
        else
            throw std::invalid_argument("Invalid side " + sideText + lineInfo());

        maxHistId = std::max(maxHistId, orderId);

        if (actionText == "ADD") {
            if (quantity <= 0)
                continue;

            addBook(symbol);
            histSymbols[orderId] = symbol;
            m_historicalEvents.push_back(std::make_unique<AddPersonalOrderEvent>(ts,
                        Order{orderId, symbol, ts, side, quantity, OrderType::Limit, price, false}));
        }
        else if (actionText == "CANCEL") {
            auto it = histSymbols.find(orderId);
            if (it == histSymbols.end())
                continue;

            m_historicalEvents.push_back(std::make_unique<CancelHistoricalOrderEvent>(ts, orderId, it->second));
            histSymbols.erase(it);
        }
        else if (actionText == "MODIFY") {
            auto it = histSymbols.find(orderId);
            if (it == histSymbols.end())
                continue;

            m_historicalEvents.push_back(std::make_unique<ModifyHistoricalOrderEvent>(ts, orderId, quantity,
                                                                                      price, it->second));
        }
        else
            throw std::invalid_argument("Invalid action " + actionText + lineInfo());
    }

    m_historicalRows = lineNumber;
    Order::m_nextId = std::max(Order::m_nextId, maxHistId);

    std::ranges::stable_sort(m_historicalEvents,
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

    if (strategyActions.has_value()) {
        for (auto& action : *strategyActions) {
            action->addLatency(m_sendLatency);
            m_personalEvents.push(std::move(action));
        }
    }
}