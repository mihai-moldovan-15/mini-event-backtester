#pragma once
#include "Types.hpp"
#include <string>

class Fill {
private:
    Symbol m_symbol{};
    OrderId m_orderId{};
    Timestamp m_ts{};
    Side m_side{};
    Quantity m_quantity{};
    Price m_price{};

public:
    Fill() = default;
    Fill(SymbolView symbol, Timestamp ts, OrderId orderId, Side side, Quantity quantity, Price price);
    [[nodiscard]] const Symbol& getSymbol() const { return m_symbol; }
    [[nodiscard]] Price getPrice() const { return m_price; }
    [[nodiscard]] Side getSide() const { return m_side; }
    [[nodiscard]] Quantity getQuantity() const { return m_quantity; }
    [[nodiscard]] OrderId getOrderId() const { return m_orderId; }
    friend std::ostream& operator<<(std::ostream& out, const Fill& fill);
};