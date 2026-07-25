#pragma once
#include "Types.hpp"
#include <optional>
#include <string>
#include <iostream>

class Order {
private:
    OrderId m_id{};
    Symbol m_symbol{};
    Timestamp m_ts{};
    Side m_side{};
    Quantity m_quantity{};
    OrderType m_type{};
    std::optional<Price> m_limitPrice{};
    bool m_isOwn{};

    inline static std::int64_t m_nextId{};

    Order(OrderId orderId, SymbolView symbol, Timestamp ts, Side side, Quantity quantity, OrderType type,
          std::optional<Price> limitPrice, bool isOwn);

    friend class OrderBook;
public:
    Order() = default;
    Order(SymbolView symbol, Timestamp ts, Side side, Quantity quantity, OrderType type,
          std::optional<Price> limitPrice = std::nullopt, bool isOwn = false);

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
    friend std::istream& operator>>(std::istream& in, Order& order);
};
