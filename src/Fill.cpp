#include "Types.hpp"
#include "Fill.hpp"
#include <iostream>


Fill::Fill(SymbolView symbol, Timestamp ts, OrderId orderId, Side side, Quantity quantity, Price price) :
            m_id(++m_nextId), m_symbol(symbol), m_ts(ts), m_orderId(orderId),
            m_side(side), m_quantity(quantity), m_price(price) {}

std::ostream& operator<<(std::ostream& out, const Fill& fill) {
    out << "Fill: ";
    if (fill.m_side == Side::Buy)
        out << "bought ";
    else
        out << "sold ";

    out << fill.m_quantity << ' ' << fill.m_symbol << ' ' << "at " << fill.m_price;
    return out;
}