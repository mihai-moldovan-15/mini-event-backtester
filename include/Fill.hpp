#pragma once
#include "Types.hpp"
#include <string>

class Fill {
private:
    FillId m_id{};
    Symbol m_symbol{};
    OrderId m_orderId{};
    Timestamp m_ts{};
    Side m_side{};
    Quantity m_quantity{};
    Price m_price{};
    inline static FillId m_nextId{};

public:
    Fill() = default;
    Fill(SymbolView symbol, Timestamp ts, OrderId orderId, Side side, Quantity quantity, Price price);
    const Symbol& getSymbol() const { return m_symbol; }
    Price getPrice() const { return m_price; }
    Side getSide() const { return m_side; }
    Quantity getQuantity() const { return m_quantity; }
    OrderId getOrderId() const { return m_orderId; }
    friend std::ostream& operator<<(std::ostream& out, const Fill& fill);
};