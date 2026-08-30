#include "Order.hpp"

#include <algorithm>

#include "Types.hpp"
#include <stdexcept>
#include <optional>
#include <iostream>

Order::Order(SymbolView symbol, Timestamp ts, Side side, Quantity quantity, OrderType type,
          std::optional<Price> limitPrice, bool isOwn) :
        m_id(++m_nextId), m_symbol(symbol), m_ts(ts), m_side(side), m_quantity(quantity), m_type(type),
        m_limitPrice(limitPrice), m_isOwn(isOwn)
{
    if (type == OrderType::Limit && !limitPrice)
        throw std::invalid_argument("Limit order requires a price");
    if (type == OrderType::Limit && limitPrice < 0)
        throw std::invalid_argument("Limit order cannot have negative price");
    if (quantity <= 0)
        throw std::invalid_argument("Quantity must be positive");
}

Order::Order(OrderId id, SymbolView symbol, Timestamp ts, Side side, Quantity quantity, OrderType type,
      std::optional<Price> limitPrice, bool isOwn)
    : m_id(id), m_symbol(symbol), m_ts(ts), m_side(side), m_quantity(quantity), m_type(type),
      m_limitPrice(limitPrice), m_isOwn(isOwn)
{
    if (type == OrderType::Limit && !limitPrice)
        throw std::invalid_argument("Limit order requires a price");
    if (type == OrderType::Limit && limitPrice < 0)
        throw std::invalid_argument("Limit order cannot have negative price");
    if (quantity <= 0)
        throw std::invalid_argument("Quantity must be positive");
}

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

std::istream& operator>>(std::istream& in, Order& order) {
    const Symbol symbol{"AAA"};/// momentan exista un singur simbol
    //in >> symbol;

    Timestamp ts{};
    in >> ts;

    if (in.eof())
        return in;

    Side side = [&]() {
        std::string s;
        in >> s;
        std::ranges::transform(s, s.begin(), ::toupper);

        if (s == "BUY")
            return Side::Buy;
        if (s == "SELL")
            return Side::Sell;

        throw std::invalid_argument("Invalid side " + s);
    }();

    Quantity quantity{};
    in >> quantity;

    OrderType type = [&]() {
        std::string s;
        in >> s;
        std::ranges::transform(s, s.begin(), ::toupper);
        if (s == "LIMIT")
            return OrderType::Limit;
        if (s == "MARKET")
            return OrderType::Market;

        throw std::invalid_argument("Invalid order type " + s);
    }();

    std::optional<Price> limitPrice{};
    if (type == OrderType::Limit) {
        Price p;
        in >> p;
        limitPrice = p;
    }

    if (!in)
        throw std::invalid_argument("Error reading from stream");

    order = Order(symbol, ts, side, quantity, type, limitPrice);

    return in;
}