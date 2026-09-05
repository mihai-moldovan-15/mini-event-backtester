#include "AStrategy.hpp"

std::optional<std::vector<std::unique_ptr<Event>>> FairPriceStrategy::onTime(
    Timestamp ts,
    const std::unordered_map<Symbol, OrderBook>& orderBooks,
    const Portfolio& portfolio,
    const std::vector<ResponseEvent>& eventsSinceLastCall)
{
    return std:: nullopt;
}