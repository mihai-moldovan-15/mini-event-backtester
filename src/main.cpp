#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include <string_view>
#include <map>
#include <optional>

using Timestamp = std::int64_t;//stored in nanoseconds
using Price = std::int64_t;//price in cents
using OrderId = std::int64_t;
using FillId = std::int64_t;


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
    std::string m_symbol{};///string momentan, poate un id e mai bun
    Timestamp m_ts{};
    Side m_side{};
    std::int32_t m_quantity{};///double for buying fractional
    OrderType m_type{};
    std::optional<Price> m_limitPrice{};

    inline static std::int64_t m_nextId{};///inline daca pun clasele intr un header mai tarziu, nu stiu inca
public:
    Order(std::string_view symbol, Timestamp ts, Side side, std::int32_t quantity, OrderType type, std::optional<double> limitPrice = std::nullopt) :
        m_id(++m_nextId), m_symbol(symbol), m_ts(ts), m_side(side), m_quantity(quantity), m_type(type), m_limitPrice(limitPrice)
    {
        if (type == OrderType::Limit && !limitPrice)
            throw std::invalid_argument("Limit order requires a price");

        if (quantity <= 0)
            throw std::invalid_argument("Quantity must be positive");
    }

    Order(Order&&) = default;
    Order& operator=(Order&&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& out, const Order& order) {
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
};


class Fill {
private:
    FillId m_id{};
    std::string m_symbol{};///string momentan, poate un id e mai bun
    Timestamp m_ts{};
    Side m_side{};
    std::int32_t m_quantity{};///double for buying fractional
    Price m_price{};

public:
    Fill() = default;
    Fill(Fill&&) = default;
    Fill& operator=(Fill&&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& out, const Fill& fill) {
        out << "Fill: ";
        if (fill.m_side == Side::Buy)
            out << "bought ";
        else
            out << "sold ";

        out << fill.m_quantity << ' ' << fill.m_symbol << ' ' << "at " << fill.m_price;
        return out;
    }
};

class Book {

};

int main() {
    std::cout << "Mini Event Backtester" << std::endl;
    return 0;
}