#pragma once
#include "Strategy.hpp"
#include "Portfolio.hpp"

class FairPriceStrategy : public Strategy {
private:
    struct StrategyConfig {
        double bias{};
        double imbalanceWeight{};
        double baseEdgeTicks{};
        double inventoryPenaltyTicks{};
        Quantity maxPosition{};
    };

    StrategyConfig m_config{};

public:
    explicit FairPriceStrategy(const StrategyConfig &config) : m_config(config) {}

    std::optional<std::vector<std::unique_ptr<Event>>> onTime
    (Timestamp ts,
    const std::unordered_map<Symbol, OrderBook>& orderBooks,
    const Portfolio& portfolio,
    const std::vector<ResponseEvent>& eventsSinceLastCall) override;
};