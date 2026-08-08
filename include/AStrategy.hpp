#pragma once
#include "Strategy.hpp"
#include "Portfolio.hpp"

class AStrategy : public Strategy {
public:
    std::optional<std::vector<std::unique_ptr<Event>>> onTime
    (Timestamp ts,
    const std::unordered_map<Symbol, OrderBook>& orderBooks,
    const Portfolio& portfolio,
    const std::vector<ResponseEvent>& eventsSinceLastCall) override;
};