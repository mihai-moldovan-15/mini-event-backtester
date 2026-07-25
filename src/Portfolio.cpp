#include "Portfolio.hpp"
#include "OrderBook.hpp"
#include <algorithm>

void Portfolio::applyFill(const Fill &fill) {
    PositionState& pos = m_positions[fill.getSymbol()];
    Quantity qty = fill.getQuantity();
    Price price = fill.getPrice();
    Position signedQty = qty;
    if (fill.getSide() == Side::Sell)
        signedQty *= -1;

    m_availableCash += fill.getSide() == Side::Sell ? price * qty : -price * qty;

    bool opening = pos.quantity == 0;
    bool sameDirection = pos.quantity * signedQty > 0;

    if (opening || sameDirection) { ///long longer, short shorte, weighted avg is good enough, pierderi la impartire
        Price totalCost = price * signedQty + pos.quantity * pos.avgEntryPrice;
        pos.quantity += signedQty;
        pos.avgEntryPrice = pos.quantity != 0 ? (totalCost / pos.quantity) : 0;

        return ;
    }

    Position closedQty = std::min(std::abs(signedQty), std::abs(pos.quantity));
    int sign = (pos.quantity > 0) ? 1 : -1;
    pos.realizedPnL += closedQty * (price - pos.avgEntryPrice) * sign;

    Position oldQuantity = pos.quantity;
    pos.quantity += signedQty;

    bool flipped = (oldQuantity * pos.quantity < 0) && pos.quantity != 0;
    ///putem fie sa ramanem flat, fie sa trecem de la long la short sau invers, avgEntryPrice este price ul actual
    if (flipped)
        pos.avgEntryPrice = price;
    else if (pos.quantity == 0)
        pos.avgEntryPrice = 0;
}

///numai simulatorul are toate orderbookurile, nu pot include din OrderBook
Cash Portfolio::getEquity(const std::unordered_map<Symbol, OrderBook>& books) const {
    Cash equity = m_availableCash;
    for (const auto& [symbol, pos] : m_positions) {
        auto it = books.find(symbol);
        if (it != books.end())
            equity += pos.quantity * it->second.getMarkPrice();
    }
    return equity;
}

Position Portfolio::getPosition(const Symbol& symbol) const {
    auto it = m_positions.find(symbol);
    return (it != m_positions.end()) ? it->second.quantity : Position{};
}

