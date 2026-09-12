#include "FairPriceStrategy.hpp"

std::optional<std::vector<std::unique_ptr<Event>>> FairPriceStrategy::onTime(
    Timestamp ts,
    const std::unordered_map<Symbol, OrderBook>& orderBooks,
    const Portfolio& portfolio,
    const std::vector<ResponseEvent>& eventsSinceLastCall)
{
    std::vector<std::unique_ptr<Event>> strategyEvents{};
    constexpr Quantity orderQuantity{1};

    for (const auto& [symbol, orderBook] : orderBooks) {
        const Price bestBid = orderBook.getBestBid();
        const Price bestAsk = orderBook.getBestAsk();

        if (bestBid <= 0 || bestAsk <= 0)
            continue;

        const Quantity bestBidQuantity = orderBook.getBestBidQuantity();
        const Quantity bestAskQuantity = orderBook.getBestAskQuantity();
        const Quantity totalQuantity = bestBidQuantity + bestAskQuantity;

        if (totalQuantity <= 0)
            continue;

        const Position position = portfolio.getPosition(symbol);

        const double midPrice = (bestBid + bestAsk) / 2.0;
        const double imbalance = static_cast<double>(bestBidQuantity - bestAskQuantity) / totalQuantity;
        const double fairPrice = midPrice + m_config.bias + m_config.imbalanceWeight * imbalance;

        const double buyEdge = fairPrice - bestAsk;
        const double sellEdge = bestBid - fairPrice;

        const double requiredBuyEdge = m_config.baseEdgeTicks + position * m_config.inventoryPenaltyTicks;
        const double requiredSellEdge = m_config.baseEdgeTicks - position * m_config.inventoryPenaltyTicks;


        const bool canAffordBuy = portfolio.getAvailableCash() >=
                                  (bestAsk + portfolio.getCommissionPerShare()) * orderQuantity;
                                  
        if (canAffordBuy && position < m_config.maxPosition && buyEdge >= requiredBuyEdge)
            strategyEvents.push_back(std::make_unique<AddPersonalOrderEvent>(ts,
                    Order{symbol, ts, Side::Buy, orderQuantity, OrderType::Limit, bestAsk, true}));
        else if (position > -m_config.maxPosition && sellEdge >= requiredSellEdge)
            strategyEvents.push_back(std::make_unique<AddPersonalOrderEvent>(ts,
                    Order{symbol, ts, Side::Sell, orderQuantity, OrderType::Limit, bestBid, true}));
    }

    if (strategyEvents.empty())
        return std::nullopt;

    return strategyEvents;
}
