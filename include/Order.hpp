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
    friend class Simulator;
public:
    Order() = default;
    Order(SymbolView symbol, Timestamp ts, Side side, Quantity quantity, OrderType type,
          std::optional<Price> limitPrice = std::nullopt, bool isOwn = false);

    [[nodiscard]] OrderId getOrderId() const { return m_id; }
    [[nodiscard]] const Symbol& getSymbol() const { return m_symbol; }
    [[nodiscard]] Timestamp getTimeStamp() const { return m_ts; }
    [[nodiscard]] Side getOrderSide() const { return m_side; }
    [[nodiscard]] Quantity getQuantity() const { return m_quantity; }
    [[nodiscard]] OrderType getOrderType() const { return m_type; }
    [[nodiscard]] const std::optional<Price>& getLimitPrice() const { return m_limitPrice; }
    [[nodiscard]] bool isOwn() const { return m_isOwn; }

    void setQuantity(Quantity newQuantity) { m_quantity = newQuantity; }

    friend std::ostream& operator<<(std::ostream& out, const Order& order);
    //friend std::istream& operator>>(std::istream& in, Order& order);
};
