#include <iostream>
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <vector>
#include <string>
#include <string_view>
#include <sstream>
#include <unordered_map>
#include <map>
#include <optional>
#include <queue>
#include <compare>
#include <memory>
#include <list>

using Timestamp = std::int64_t;//stored in nanoseconds
using SequenceNumber = std::int64_t;//differentiate between buys/sells when modifying an order
using Price = std::int64_t;//price in cents
using Position = std::int64_t;
using Cash = std::int64_t;
using OrderId = std::int64_t;
using FillId = std::int64_t;
using Quantity = std::int32_t;
using Symbol = std::string;
using SymbolView = std::string_view;

enum class Side {
    Buy,
    Sell,
};

enum class OrderType {
    Market,
    Limit,
};

class Order {
private:
    OrderId m_id{};
    Symbol m_symbol{};///string momentan, poate un id e mai bun
    Timestamp m_ts{};
    Side m_side{};
    Quantity m_quantity{};
    OrderType m_type{};
    std::optional<Price> m_limitPrice{};
    bool m_isOwn{};

    inline static std::int64_t m_nextId{};///inline daca pun clasele intr un header mai tarziu, nu stiu inca
public:
    Order() = default;
    Order(SymbolView symbol, Timestamp ts, Side side, Quantity quantity, OrderType type,
          std::optional<Price> limitPrice = std::nullopt, bool isOwn = false) :
        m_id(++m_nextId), m_symbol(symbol), m_ts(ts), m_side(side), m_quantity(quantity), m_type(type),
        m_limitPrice(limitPrice), m_isOwn(isOwn)
    {
        if (type == OrderType::Limit && !limitPrice)
            throw std::invalid_argument("Limit order requires a price");

        if (quantity <= 0)
            throw std::invalid_argument("Quantity must be positive");
    }

    OrderId getOrderId() const { return m_id; }
    const Symbol& getSymbol() const { return m_symbol; }
    Timestamp getTimeStamp() const { return m_ts; }
    Side getOrderSide() const { return m_side; }
    Quantity getQuantity() const { return m_quantity; }
    OrderType getOrderType() const { return m_type; }
    const std::optional<Price>& getLimitPrice() const { return m_limitPrice; }
    bool isOwn() const { return m_isOwn; }

    void setQuantity(Quantity newQuantity) { m_quantity = newQuantity; }
    void setLimitPrice(std::optional<Price> newLimitPrice) { if (newLimitPrice) m_limitPrice = newLimitPrice; }

    friend std::ostream& operator<<(std::ostream& out, const Order& order);
};

std::ostream& operator<<(std::ostream& out, const Order& order) {
    out << "Order: ";
    if (order.m_side == Side::Buy)
        out << "buy ";
    else
        out << "sell ";

    out << order.m_quantity << ' ' << order.m_symbol << ' ' << "at ";
    if (order.m_limitPrice)
        out << order.m_limitPrice.value();
    else
        out << "market";

    return out;
}


class Fill {
private:
    FillId m_id{};
    Symbol m_symbol{};///string momentan, poate un id e mai bun
    OrderId m_orderId{};
    Timestamp m_ts{};
    Side m_side{};
    Quantity m_quantity{};
    Price m_price{};
    inline static FillId m_nextId{};

public:
    Fill() = default;
    Fill(SymbolView symbol, Timestamp ts, OrderId orderId, Side side, Quantity quantity, Price price) :
        m_id(++m_nextId), m_symbol(symbol), m_ts(ts), m_orderId(orderId), m_side(side), m_quantity(quantity), m_price(price) {}

    const Symbol& getSymbol() const { return m_symbol; }
    Price getPrice() const { return m_price; }
    Side getSide() const { return m_side; }
    Quantity getQuantity() const { return m_quantity; }
    OrderId getOrderId() const { return m_orderId; }
    friend std::ostream& operator<<(std::ostream& out, const Fill& fill);
};

std::ostream& operator<<(std::ostream& out, const Fill& fill) {
    out << "Fill: ";
    if (fill.m_side == Side::Buy)
        out << "bought ";
    else
        out << "sold ";

    out << fill.m_quantity << ' ' << fill.m_symbol << ' ' << "at " << fill.m_price;
    return out;
}


///initial era std::deque<Order> orders{}; schimbat la lista
struct BookLevel {
    std::list<Order> orders{};
    Quantity totalQuantity{};
};

struct BookSnapshot {///starea bookului la un anumit timp
    Timestamp ts{};
    Symbol symbol{};
    std::map<Price, BookLevel, std::greater<>> bids{};
    std::map<Price, BookLevel> asks{};
    friend std::ostream& operator<<(std::ostream&, const BookSnapshot& book_snapshot);
};


class OrderBook {//starea actuala a Bookului
private:
    Symbol m_symbol{};
    std::map<Price, BookLevel> m_asks;
    std::map<Price, BookLevel, std::greater<>> m_bids;
public:
    void validate() const;

public:
    explicit OrderBook(SymbolView symbol): m_symbol(symbol) {}
    BookSnapshot snapshot(Timestamp ts) const;
    Price getBestAsk() const { return (m_asks.empty()) ? 0: m_asks.begin()->first; }
    Price getBestBid() const { return (m_bids.empty()) ? 0: m_bids.begin()->first; }
    const auto& getAsks() const { return m_asks; }
    auto& asks() { return m_asks; }
    auto& bids() { return m_bids; }
    const auto& getBids() const { return m_bids; }
    const Symbol& getSymbol() const { return m_symbol; }

    void removeLevel(Side side, Price price) { (side == Side::Buy) ? m_bids.erase(price) : m_asks.erase(price); }
};

///se poate valida numai dupa ce bookul e umplut(biduri + askuri)
void OrderBook::validate() const {
    if (m_symbol.empty())
        throw std::invalid_argument("Missing book symbol");

    if (!m_asks.empty() && !m_bids.empty()) {
        if (m_asks.begin()->first < m_bids.begin()->first)
            throw std::invalid_argument("Book cannot be crossed");

        if (m_bids.rbegin()->first < 0)
            throw std::invalid_argument("Bid prices cannot be negative");

        for (const auto& [price, level] : m_bids)
            if (level.totalQuantity < 0)
                throw std::invalid_argument("Bid quantities cannot be negative");

        if (m_asks.begin()->first < 0)
            throw std::invalid_argument("Ask prices cannot be negative");

        for (const auto& [price, level] : m_asks)
            if (level.totalQuantity < 0)
                throw std::invalid_argument("Ask quantities cannot be negative");
    }
}

BookSnapshot OrderBook::snapshot(Timestamp ts) const {
    BookSnapshot snapshot{};
    snapshot.symbol = m_symbol;
    snapshot.ts = ts;

    for (auto const& [price, bookLevel]: m_bids)
        snapshot.bids.emplace(price, bookLevel);
    for (auto const& [price, bookLevel]: m_asks)
        snapshot.asks.emplace(price, bookLevel);

    return snapshot;
}

class Simulator;
class Event {
protected:
    Timestamp m_sentTs{};
    SequenceNumber m_sqNum{};
    inline static SequenceNumber m_nextSqNum{};
public:
    Event(Timestamp sentTs, SequenceNumber sqNum = ++m_nextSqNum): m_sentTs(sentTs), m_sqNum(sqNum) {}
    Event& operator=(const Event&) = delete;
    Timestamp getTimestamp() const { return m_sentTs; }
    SequenceNumber getSequenceNumber() const { return m_sqNum; }

    virtual void execute(Simulator&) = 0;
    virtual ~Event() = default;
};

struct eventCompare {
    bool operator()(const std::unique_ptr<Event>& a,
                    const std::unique_ptr<Event>& b) const {
        if (a->getTimestamp() != b->getTimestamp())
            return a->getTimestamp() > b->getTimestamp();

        return a->getSequenceNumber() > b->getSequenceNumber();
    }
};

class TimerEvent: public Event {
public:
    explicit TimerEvent(Timestamp sentTs): Event(sentTs) {}
    void execute(Simulator& simulator) override;
};

class AddOrderEvent: public Event{
private:
    Order m_order{};
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

class FillEvent: public Event {
private:
    Fill m_fill{};
public:
    FillEvent(Timestamp sentTs, Fill fill): Event(sentTs), m_fill(std::move(fill)) {}
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

struct PositionState {
    Position quantity{};//sign of position tells if it is long, short or neutral
    Price averageEntryPrice{};
};

/*
int signed_quantity(const Fill& fill) {
    if (fill.getSide() == Side::Buy) {
        return fill.getQuantity();
    }
    return -fill.getQuantity();
}
*/

struct Portfolio {
    Cash availableCash{};
    std::unordered_map<Symbol, PositionState> positions;
};


struct OrderLocation {
    BookLevel* level;
    std::list<Order>::iterator it;///era List<Order>::Node* inainte, List era o lista simplu inlantuita
                                  ///pointer direct la ordinul propriu, nu la cel dinainte.
                                  ///Stergere: level->orders.erase(it);
};

class Simulator {
private:
    Timestamp m_currentTime{};
    const Timestamp m_latency{ 3'000'000 };
    Timestamp m_timeDelta{ 2'000'000 };
    Timestamp m_endTime{ 1'000'000'000 };
    Cash m_initialCash{};
    Portfolio m_portfolio{};
    std::unordered_map<Symbol, OrderBook> m_orderBooks{};
    std::priority_queue<std::unique_ptr<Event>, std::vector<std::unique_ptr<Event>>, eventCompare> m_events{};
    std::unordered_map<OrderId, OrderLocation> m_activeOrders;
    std::vector<Fill> m_fillsRecord{};
public:
    void run();

    Simulator(Cash initialCash, Timestamp endTime = 1'000'000'000) : m_initialCash(initialCash), m_portfolio{initialCash, {}}, m_endTime(endTime){}
    Timestamp addOrder(const Order& order);
    Timestamp modifyOrder(OrderId orderId, Quantity newQuantity, std::optional<Price> newLimitPrice);
    Timestamp cancelOrder(OrderId orderId);

    Price getMarkPrice(const Symbol& symbol) const;
    Price getSpread(const OrderBook& book) const;
    Cash getEquity() const;
    Cash getPnL() const;
    Cash getCash() const;
    Timestamp getCurrTimeStamp() const { return m_currentTime; }
    const std::vector<Fill>& getFillsRecord() const { return m_fillsRecord; }
    Position getPosition(const Symbol& symbol) const;

    void processAddOrder(const Order& order);
    void processCancelOrder(OrderId orderid);
    void processFill(const Fill& fill);
    void processModifyOrder(OrderId id, Quantity newQty, std::optional<Price> newPrice);
    void scheduleTimer();
    void recordTrade(const Order& incoming, const Order& existing, Price price, Quantity qty);
    void addToBook(const Order& order, OrderBook& book);
    void addBook(const Symbol& symbol);


    const OrderBook& getOrderBook(const Symbol& symbol) const{
        return m_orderBooks.at(symbol);
    }
};

void AddOrderEvent::execute(Simulator& s)    { s.processAddOrder(m_order); }
void CancelOrderEvent::execute(Simulator& s) { s.processCancelOrder(m_orderId); }
void TimerEvent::execute(Simulator& s)       { s.scheduleTimer(); }
void ModifyOrderEvent::execute(Simulator& s) { s.processModifyOrder(m_orderId, m_newQuantity, m_newLimitPrice); }

void Simulator::scheduleTimer() {
    Timestamp next = m_currentTime + m_timeDelta;
    if (next > m_endTime)
        return;
    m_events.push(std::make_unique<TimerEvent>(next));
}

void Simulator::processFill(const Fill &fill) {
    PositionState& pos = m_portfolio.positions[fill.getSymbol()];

    if (fill.getSide() == Side::Buy) {
        m_portfolio.availableCash -= fill.getQuantity() * fill.getPrice();
        pos.quantity += fill.getQuantity();
    }
    else {
        m_portfolio.availableCash += fill.getQuantity() * fill.getPrice();
        pos.quantity -= fill.getQuantity();
    }

    m_fillsRecord.push_back(fill);
}

void Simulator::processModifyOrder(OrderId id, Quantity newQty, std::optional<Price> newPrice) {
    auto it = m_activeOrders.find(id);
    if (it == m_activeOrders.end())
        return;

    const Order& initial = *it->second.it;
    Order newOrder{ initial.getSymbol(), m_currentTime, initial.getOrderSide(), newQty,
                    initial.getOrderType(), newPrice ? newPrice : initial.getLimitPrice(),
                    initial.isOwn() };

    processCancelOrder(id);
    processAddOrder(newOrder);
}

void FillEvent::execute(Simulator& simulator) {
    simulator.processFill(m_fill);
}

void Simulator::run() {
    while (!m_events.empty() && m_currentTime < m_endTime) {
        auto event = std::move(const_cast<std::unique_ptr<Event>&>(m_events.top()));///pq returneaza const T&, scoatem const ul
        m_events.pop();

        m_currentTime = event->getTimestamp();
        event->execute(*this);
    }
}

Timestamp Simulator::addOrder(const Order& order) {
    if (m_orderBooks.find(order.getSymbol()) == m_orderBooks.end())
        throw std::invalid_argument("Unknown symbol: " + order.getSymbol());
    if (order.getOrderType() == OrderType::Limit && *order.getLimitPrice() <= 0)
        throw std::invalid_argument("Limit price must be positive");

    Timestamp expectedTime{ m_currentTime + m_latency };
    m_events.push(std::make_unique<AddOrderEvent>(expectedTime, order));

    return expectedTime;
}

Timestamp Simulator::cancelOrder(OrderId orderId) {
    Timestamp expectedTime{ m_currentTime + m_latency };

    m_events.push(std::make_unique<CancelOrderEvent>(expectedTime, orderId));

    return expectedTime;
}

Timestamp Simulator::modifyOrder(OrderId id, Quantity q, std::optional<Price> p) {
    Timestamp expectedTime{ m_currentTime + m_latency };
    m_events.push(std::make_unique<ModifyOrderEvent>(expectedTime, id, q, p));

    return expectedTime;
}


Price Simulator::getMarkPrice(const Symbol& symbol) const {
    auto it{ m_orderBooks.find(symbol) };
    if (it == m_orderBooks.end())
        throw std::runtime_error("Unknown symbol");

    Price bestAskPrice{ it->second.getBestAsk() };
    Price bestBidPrice{ it->second.getBestBid() };

    if (!bestAskPrice && !bestBidPrice)
        throw std::runtime_error("No available bids or asks for " + symbol);

    if (!bestAskPrice)
        return bestBidPrice;

    if (!bestBidPrice)
        return bestAskPrice;

    return (bestBidPrice + bestAskPrice) / 2;
}

Price Simulator::getSpread(const OrderBook& book) const {
    if (book.getAsks().empty() || book.getBids().empty())
        throw std::runtime_error("Cannot calculate spread without bids/asks");
    return book.getBestAsk() - book.getBestBid();
}

Cash Simulator::getEquity() const {
    Cash equity{ this->m_portfolio.availableCash };
    for (const auto& [symbol, positionState]: this->m_portfolio.positions)
        equity += positionState.quantity * getMarkPrice(symbol);

    return equity;
}

Cash Simulator::getPnL() const {
    return getEquity() - m_initialCash;
}

Cash Simulator::getCash() const {
    return m_portfolio.availableCash;
}

Position Simulator::getPosition(const Symbol& symbol) const {
    auto it = m_portfolio.positions.find(symbol);
    return (it == m_portfolio.positions.end()) ? 0 : it->second.quantity;
}

void Simulator::processCancelOrder(OrderId orderid) {
    auto itActive = m_activeOrders.find(orderid);
    if (itActive == m_activeOrders.end())
        return;

    OrderLocation loc = itActive->second;
    const Symbol symbol = loc.it->getSymbol();
    const Price price = *loc.it->getLimitPrice();
    const Side side = loc.it->getOrderSide();

    loc.level->totalQuantity -= loc.it->getQuantity();
    loc.level->orders.erase(loc.it);
    if (loc.level->orders.empty()) {
        auto& book = m_orderBooks.at(symbol);
        book.removeLevel(side, price);
    }

    m_activeOrders.erase(itActive);
}

void Simulator::recordTrade(const Order &incoming, const Order &existing, Price price, Quantity qty) {
    if (incoming.isOwn())
        m_events.push(std::make_unique<FillEvent>(
            m_currentTime + m_latency,
            Fill{ incoming.getSymbol(), m_currentTime, incoming.getOrderId(), incoming.getOrderSide(), qty, price }));

    if (existing.isOwn())
        m_events.push(std::make_unique<FillEvent>(
            m_currentTime + m_latency,
            Fill{ existing.getSymbol(), m_currentTime, existing.getOrderId(), existing.getOrderSide(), qty, price }));
}

void Simulator::addToBook(const Order &order, OrderBook &book) {
    Price price{ *order.getLimitPrice() };
    BookLevel& level{(order.getOrderSide() == Side::Buy) ? book.bids()[price] : book.asks()[price]};

    level.orders.push_back(order);
    level.totalQuantity += order.getQuantity();

    m_activeOrders[order.getOrderId()] = OrderLocation{ &level, std::prev(level.orders.end()) };
}

void Simulator::addBook(const Symbol& symbol) {
    if (m_orderBooks.find(symbol) == m_orderBooks.end())
        m_orderBooks.emplace(symbol, OrderBook(symbol));
}

///putin redundanta, se poate simplifica
void Simulator::processAddOrder(const Order& order) {
    Order newOrder = order;
    auto& book = m_orderBooks.at(order.getSymbol());
    if (newOrder.getOrderSide() == Side::Buy) {
        ///trebuie tratat cazul in care nu avem suficient cash pentru a plasa o anumita comanda

        auto& levels = book.asks();
        auto it = levels.begin();
        while (newOrder.getQuantity() > 0 && it != levels.end()) {
            Price existingPrice = it->first;
            if (newOrder.getOrderType() == OrderType::Limit && *newOrder.getLimitPrice() < existingPrice)
                break;

            BookLevel& level = it->second;
            auto ordIt = level.orders.begin();

            while (newOrder.getQuantity() > 0 && ordIt != level.orders.end()) {
                Order& existing = *ordIt;
                Quantity qty = std::min(newOrder.getQuantity(), existing.getQuantity());

                recordTrade(newOrder, existing, existingPrice, qty);

                newOrder.setQuantity(newOrder.getQuantity() - qty);
                existing.setQuantity(existing.getQuantity() - qty);
                level.totalQuantity -= qty;

                if (existing.getQuantity() == 0) {
                    m_activeOrders.erase(existing.getOrderId());
                    ordIt = level.orders.erase(ordIt);
                }
                else
                    ++ordIt;
            }

            it = level.orders.empty() ? levels.erase(it) : std::next(it);
        }
    }
    else {///Side::Sell
        auto& levels = book.bids();
        auto it = levels.begin();
        while (newOrder.getQuantity() > 0 && it != levels.end()) {
            Price existingPrice = it->first;
            if (newOrder.getOrderType() == OrderType::Limit && *newOrder.getLimitPrice() > existingPrice)
                break;

            BookLevel& level = it->second;
            auto ordIt = level.orders.begin();

            while (newOrder.getQuantity() > 0 && ordIt != level.orders.end()) {
                Order& existing = *ordIt;
                Quantity qty = std::min(newOrder.getQuantity(), existing.getQuantity());

                recordTrade(newOrder, existing, existingPrice, qty);

                newOrder.setQuantity(newOrder.getQuantity() - qty);
                existing.setQuantity(existing.getQuantity() - qty);
                level.totalQuantity -= qty;

                if (existing.getQuantity() == 0) {
                    m_activeOrders.erase(existing.getOrderId());
                    ordIt = level.orders.erase(ordIt);
                }
                else
                    ++ordIt;
            }

            it = level.orders.empty() ? levels.erase(it) : std::next(it);
        }
    }

    if (newOrder.getQuantity() > 0 && newOrder.getOrderType() == OrderType::Limit)
        addToBook(newOrder, book);
}

static std::string fmtPrice(Price p) {
    //Claude Opus 4.8 Medium effort
    std::ostringstream os;
    os << p / 100 << '.' << std::setfill('0') << std::setw(2) << p % 100;
    return os.str();
}

int main() {
    Simulator S(1'000'000);

    S.addBook("ABC");
    S.addOrder(Order{"ABC", 0, Side::Sell,  40, OrderType::Limit, 10005});
    S.addOrder(Order{"ABC", 0, Side::Sell,  60, OrderType::Limit, 10010});
    S.addOrder(Order{"ABC", 0, Side::Sell,  90, OrderType::Limit, 10015});

    S.addOrder(Order{"ABC", 0, Side::Buy,   50, OrderType::Limit, 10000});
    S.addOrder(Order{"ABC", 0, Side::Buy,   75, OrderType::Limit,  9995});
    S.addOrder(Order{"ABC", 0, Side::Buy,  100, OrderType::Limit,  9990});

    S.run();//nu pot face validarea la inceput, orderele nu sunt trimise, sunt doar in coada de eventuri
            ///ar trebui sa fac la fiecare processOrder

    S.getOrderBook("ABC").validate();
    std::cout << S.getOrderBook("ABC").snapshot(1'000) << "\n\n";

    const auto& book = S.getOrderBook("ABC");
    std::cout << "Best bid: " << fmtPrice(book.getBestBid()) << '\n';
    std::cout << "Best ask: " << fmtPrice(book.getBestAsk()) << '\n';
    std::cout << "Spread: " << fmtPrice(S.getSpread(book)) << '\n';
    std::cout << "Mid: " << fmtPrice(S.getMarkPrice(book.getSymbol())) << "\n\n";


    Order buy_order{"ABC", 1'010, Side::Buy, 40, OrderType::Limit, 10005, true};
    OrderId id = buy_order.getOrderId();

    Timestamp tadd = S.addOrder(buy_order);
    S.run();
    Timestamp tmodifiy = S.modifyOrder(id, 30, 10005);
    S.run();
    Timestamp tcancel = S.cancelOrder(id);
    S.scheduleTimer();
    S.run();

    std::cout << "Add command is processed at: " << tadd << "\n";
    std::cout << "Modify command is processed at: " << tmodifiy << "\n";
    std::cout << "Cancel command is processed at: " << tcancel << "\n\n";


    std::cout << "Fills:\n";
    for (const Fill& f : S.getFillsRecord())
        std::cout << "  " << f << '\n';

    std::cout << "position = " << S.getPosition("ABC") << '\n';
    std::cout << "cash     = " << S.getCash() << '\n';
    std::cout << "equity   = " << S.getEquity() << '\n';
    std::cout << "PnL      = " << S.getPnL() << "\n\n";

    std::cout << S.getOrderBook("ABC").snapshot(S.getCurrTimeStamp());
    return 0;
}

std::ostream& operator<<(std::ostream& out, const BookSnapshot& book_snapshot) {
    //Claude Opus 4.8 Medium effort
    std::string title = "ORDER BOOK FOR " + book_snapshot.symbol;
    constexpr int totalWidth{ 60 };
    constexpr int colWidth  { 12 };

    out << std::string((totalWidth - static_cast<int>(title.size())) / 2, ' ')
        << title << "\n\n";

    out << std::left
        << std::setw(colWidth * 2) << "BIDS"
        << " | "
        << std::setw(colWidth * 2) << "ASKS" << '\n';

    out << std::setw(colWidth) << "Price"
        << std::setw(colWidth) << "Qty"
        << " | "
        << std::setw(colWidth) << "Price"
        << std::setw(colWidth) << "Qty" << '\n';

    out << std::string(totalWidth, '-') << '\n';

    auto bidIt{ book_snapshot.bids.begin() };
    auto askIt{ book_snapshot.asks.begin() };

    const size_t maxLevels{ 20 };
    size_t printed{};

    while ((bidIt != book_snapshot.bids.end() || askIt != book_snapshot.asks.end())
           && printed < maxLevels) {
        // BID SIDE
        if (bidIt != book_snapshot.bids.end()) {
            out << std::setw(colWidth) << fmtPrice(bidIt->first)
                << std::setw(colWidth) << bidIt->second.totalQuantity;
            ++bidIt;
        }
        else
            out << std::setw(colWidth * 2) << "";

        out << " | ";

        // ASK SIDE
        if (askIt != book_snapshot.asks.end()) {
            out << std::setw(colWidth) << fmtPrice(askIt->first)
                << std::setw(colWidth) << askIt->second.totalQuantity;
            ++askIt;
        }
        else
            out << std::setw(colWidth * 2) << "";

        out << '\n';
        ++printed;
    }

    if (printed == maxLevels)
        out << "... (truncated)\n";

    out << std::setfill(' ');
    return out;
}