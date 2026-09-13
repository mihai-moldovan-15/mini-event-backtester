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

    ///comisionul se plateste pe orice fill, indiferent de directie; nu intra in avgEntryPrice
    ///// REVIEW: Comisionul este dedus din ambele availableCash și realizedPnL - numără dublu costurile comisionului, făcând P&L incorect
    ///  Raspuns: getPnL() / getEquity() folosesc doar availableCash, realizedPnL nu e folosit niciodata in calcularea efectiva a P&L
    ///           nu e cel mai bun design choice

    const Cash commission = m_commissionPerShare * qty;
    m_availableCash -= commission;
    pos.realizedPnL -= commission;

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

    bool flipped = (oldQuantity * pos.quantity < 0);
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
            // REVIEW: Risc de overflow întreg - pos.quantity * getMarkPrice() poate face overflow pentru poziții mari sau prețuri folosind int64_t
            // Raspuns: Cred ca e putin probabil un overflow, as lasa codul asa
            equity += pos.quantity * it->second.getMarkPrice();
    }
    return equity;
}

Position Portfolio::getPosition(const Symbol& symbol) const {
    auto it = m_positions.find(symbol);
    return (it != m_positions.end()) ? it->second.quantity : Position{};
}

void Portfolio::liquidate(const std::unordered_map<Symbol, OrderBook>& books) {
    for (const auto& [symbol, position]: m_positions) {
        if (position.quantity == 0)
            continue;

        auto it = books.find(symbol);
        if (it == books.end())
            continue;

        bool isLong = position.quantity > 0;

        // REVIEW: Logică gresita de fallback, getMarkPrice() aruncă excepție când bid și ask sunt ambele 0, deci apelul din prima condiție va cauza crash
        // Raspuns:
        Price bestBid = it->second.getBestBid();
        Price bestAsk = it->second.getBestAsk();

        if (!bestBid && !bestAsk)
            continue;

        Price exitPrice = isLong ? bestBid : bestAsk;

        if (exitPrice == 0)
            exitPrice = it->second.getMarkPrice();

        Quantity qt = std::abs(position.quantity);
        applyFill(Fill{symbol, Timestamp{}, OrderId{}, isLong ? Side::Sell : Side::Buy, qt, exitPrice});
    }
}
