#include "AStrategy.hpp"
#include "Simulator.hpp"

int main() {
    Simulator S(1'000'000);
    S.setStrategy(std::make_unique<AStrategy>());
    S.addBook("ABC");

    S.scheduleEvent(std::make_unique<AddOrderEvent>(0, Order{"ABC", 0, Side::Sell, 40, OrderType::Limit, 10005}));
    S.scheduleEvent(std::make_unique<AddOrderEvent>(0, Order{"ABC", 0, Side::Sell, 60, OrderType::Limit, 10010}));
    S.scheduleEvent(std::make_unique<AddOrderEvent>(0, Order{"ABC", 0, Side::Sell, 90, OrderType::Limit, 10015}));

    S.scheduleEvent(std::make_unique<AddOrderEvent>(0, Order{"ABC", 0, Side::Buy, 50, OrderType::Limit, 10000}));
    S.scheduleEvent(std::make_unique<AddOrderEvent>(0, Order{"ABC", 0, Side::Buy, 75, OrderType::Limit, 9995}));
    S.scheduleEvent(std::make_unique<AddOrderEvent>(0, Order{"ABC", 0, Side::Buy, 100, OrderType::Limit, 9990}));
    S.scheduleEvent(std::make_unique<AddOrderEvent>(0, Order{"ABC", 0, Side::Buy, 100, OrderType::Limit, 1005}));
    S.scheduleEvent(std::make_unique<AddOrderEvent>(0, Order{"ABC", 0, Side::Buy, 30, OrderType::Limit, 10005}));

    S.run();

    S.getOrderBook("ABC").validate();

    std::cout << S.getOrderBook("ABC") << "\n\n";
    std::cout << "Best bid: " << fmtPrice(S.getOrderBook("ABC").getBestBid()) << '\n';
    std::cout << "Best ask: " << fmtPrice(S.getOrderBook("ABC").getBestAsk()) << '\n';
    std::cout << "Spread: " << fmtPrice(S.getOrderBook("ABC").getSpread()) << '\n';
    std::cout << "Mid: " << fmtPrice(S.getOrderBook("ABC").getMarkPrice()) << "\n\n";

    Order buy_order{"ABC", 1'010, Side::Buy, 40, OrderType::Limit, 10005, true};
    OrderId id = buy_order.getOrderId();

    S.scheduleEvent(std::make_unique<AddOrderEvent>(1000, buy_order));
    S.scheduleEvent(std::make_unique<ModifyOrderEvent>(2000, id, 15, 10005));
    S.scheduleEvent(std::make_unique<CancelOrderEvent>(3000, id));

    S.run();

    std::cout << "Fills:\n";
    for (const Fill& f : S.getFillsRecord())
        std::cout << "  " << f << '\n';

    std::cout << "position = " << S.getPortfolio().getPosition("ABC") << '\n';
    std::cout << "cash     = " << S.getPortfolio().getAvailableCash() << '\n';
    std::cout << "equity   = " << S.getPortfolio().getEquity(S.getOrderBooks()) << '\n';
    std::cout << "PnL      = " << S.getPortfolio().getPnL(S.getOrderBooks()) << "\n\n";

    std::cout << S.getOrderBook("ABC");
    return 0;
}