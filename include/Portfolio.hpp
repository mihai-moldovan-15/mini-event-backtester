#pragma once
#include "Types.hpp"
#include "Fill.hpp"
#include <unordered_map>
#include "OrderBook.hpp"

struct PositionState {
    Position quantity{};
    Price avgEntryPrice{};
    Cash realizedPnL{};
};

class Portfolio {
private:
    Cash m_initialCash{};
    Cash m_availableCash{};
    std::unordered_map<Symbol, PositionState> m_positions{};

public:
    explicit Portfolio(Cash initialCash): m_initialCash(initialCash), m_availableCash(initialCash) {}
    Cash getAvailableCash() const { return m_availableCash; }
    Cash getEquity(const std::unordered_map<Symbol, OrderBook>& books) const;
    Cash getPnL(const std::unordered_map<Symbol, OrderBook>& books) const { return getEquity(books) - m_initialCash; }
    Cash getRealizedPnL(const Symbol& symbol) { return m_positions[symbol].realizedPnL; }
    Position getPosition(const Symbol& symbol) const;

    void applyFill(const Fill& fill);
    void liquidate(const std::unordered_map<Symbol, OrderBook>& books);
};