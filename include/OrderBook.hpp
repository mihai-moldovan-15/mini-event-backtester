#pragma once
#include "Types.hpp"
#include <list>
#include <unordered_map>
#include <map>
#include "Order.hpp"
#include <vector>
#include "ResponseEvent.hpp"
#include <optional>
#include <functional>///pentru std::greater<>

struct BookLevel {
    std::list<Order> orders{};
    Quantity totalQuantity{};
};

class OrderBook {
private:
    struct OrderLocation {
        Side side;
        Price price;
        BookLevel* level;
        std::list<Order>::iterator it;
    };
    Symbol m_symbol{};
    std::map<Price, BookLevel> m_asks;
    std::map<Price, BookLevel, std::greater<>> m_bids;
    std::unordered_map<OrderId, OrderLocation> m_activeOrders{};
    void removeLevel(Side side, Price price) { (side == Side::Buy) ? m_bids.erase(price) : m_asks.erase(price); }
public:
    explicit OrderBook(SymbolView symbol): m_symbol(symbol) {}

    OrderBook(const OrderBook&) = delete;                          //BookLevel* level;
    OrderBook& operator=(const OrderBook&) = delete;               //BookLevel* level;
    OrderBook(OrderBook&&) = default;
    OrderBook& operator=(OrderBook&&) = default;

    Price getBestAsk() const { return (m_asks.empty()) ? 0: m_asks.begin()->first; }
    Price getBestBid() const { return (m_bids.empty()) ? 0: m_bids.begin()->first; }
    const auto& getAsks() const { return m_asks; }
    const auto& getBids() const { return m_bids; }
    const Symbol& getSymbol() const { return m_symbol; }

    //bool checkFill(const Order& order);///fac verificarea direct in processAddOrder

    void recordTrade(const Order& incoming, const Order& existing, Price price, Quantity qty,
                             Timestamp currentTime, std::vector<ResponseEvent>& responses);
    /// adaugarea efectiva in book, procesarea eventurilor, intorc response events
    std::vector<ResponseEvent> processAddOrder(const Order& order, Cash availableCash, Timestamp currentTime); ///aici se intampla crossingul, poate genera mai multe responseuri
    ResponseEvent processCancelOrder(OrderId orderid, Timestamp currentTime);
    std::vector<ResponseEvent> processModifyOrder(OrderId id, Quantity newQty, std::optional<Price> newPrice,
                                                   Cash availableCash, Timestamp currentTime);///add ul poate genera mai multe responseuri

    Price getMarkPrice() const;
    Price getSpread() const;

    void validate() const;
    friend std::ostream& operator<<(std::ostream& out, const OrderBook& book);
};


std::string fmtPrice(Price p);