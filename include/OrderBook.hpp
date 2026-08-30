#pragma once
#include "Types.hpp"
#include <unordered_map>
#include <map>
#include "Order.hpp"
#include <vector>
#include "ResponseEvent.hpp"
#include <optional>
#include <functional>///pentru std::greater<>

#include "../data structures/double_linked_list.hpp"

struct BookLevel {
    List<Order> orders;
    Quantity totalQuantity{};
    explicit BookLevel(NodePool<Order>* pool) : orders(pool) {}
};

class OrderBook {
private:
    struct OrderLocation {
        Side side;
        Price price;
        BookLevel* level;
        ListNode<Order>* it;
    };
    Symbol m_symbol{};
    SizeValue m_relevantLevelCount{};
    NodePool<Order> m_pool{};

    std::map<Price, BookLevel> m_relevantAsks;
    std::map<Price, BookLevel> m_otherAsks;
    std::map<Price, BookLevel, std::greater<>> m_relevantBids;
    std::map<Price, BookLevel, std::greater<>> m_otherBids;

    std::unordered_map<OrderId, OrderLocation> m_activeOrders{};
    void removeRelevantLevel(Side side, Price price) { (side == Side::Buy) ? m_relevantBids.erase(price) : m_relevantAsks.erase(price); }
    void removeOtherLevel(Side side, Price price) { (side == Side::Buy) ? m_otherBids.erase(price) : m_otherAsks.erase(price); }
    void rebalanceLevels();
public:
    explicit OrderBook(const SymbolView symbol, SizeValue relevantLevelCount = 10): m_symbol(symbol), m_relevantLevelCount(relevantLevelCount){}

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) = default;
    OrderBook& operator=(OrderBook&&) = default;

    ///verif doar bestAsks/bestBids, nu ar trebui ca relevantAsks sa fie gol si otherAsks sa contina elemente
    Price getBestAsk() const { return (m_relevantAsks.empty()) ? 0: m_relevantAsks.begin()->first; }
    Price getBestBid() const { return (m_relevantBids.empty()) ? 0: m_relevantBids.begin()->first; }

    const auto& getRelevantAsks() const { return m_relevantAsks; }
    const auto& getRelevantBids() const { return m_relevantBids; }
    const Symbol& getSymbol() const { return m_symbol; }

    //bool checkFill(const Order& order);///fac verificarea direct in processAddOrder

    void recordTrade(const Order& incoming, const Order& existing, Price price, Quantity qty,
                             Timestamp currentTime, std::vector<ResponseEvent>& responses) const;
    /// adaugarea efectiva in book, procesarea eventurilor, intorc response events
    std::vector<ResponseEvent> processAddOrder(const Order& order, Cash availableCash, Timestamp currentTime); ///aici se intampla crossingul, poate genera mai multe responseuri
    ResponseEvent processCancelOrder(OrderId orderid, Timestamp currentTime, bool isOwnRequest);
    std::vector<ResponseEvent> processModifyOrder(OrderId id, Quantity newQty, std::optional<Price> newPrice,
                                                   Cash availableCash, Timestamp currentTime, bool isOwnRequest);///add ul poate genera mai multe responseuri

    Price getMarkPrice() const;
    Price getSpread() const;

    void validate() const;
    friend std::ostream& operator<<(std::ostream& out, const OrderBook& book);
};


std::string fmtPrice(Price p);