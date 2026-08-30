#include "AStrategy.hpp"
#include "Simulator.hpp"

int main() {
    AStrategy strategy{};

    try {
        Simulator S(std::make_unique<AStrategy>(strategy), 1'000'000, 21'000'000'000);
        S.loadHistoricalEvents("../data/new_historical_events.txt");
        S.scheduleTimer();
        S.run();

        std::cout << "Fills:\n";
        for (const Fill& f : S.getFillsRecord())
            std::cout << "  " << f << '\n';

        std::cout << "position = " << S.getPortfolio().getPosition("ABC") << '\n';
        std::cout << "cash     = " << S.getPortfolio().getAvailableCash() << '\n';
        std::cout << "equity   = " << S.getPortfolio().getEquity(S.getOrderBooks()) << '\n';
        std::cout << "PnL      = " << S.getPortfolio().getPnL(S.getOrderBooks()) << "\n\n";

        for (const auto& [name, _] : S.getOrderBooks())
            std::cout << S.getOrderBook(name) << "\n\n\n\n\n";
    }
    catch(...) {
        std::cout << "Exception occured";
    }

    return 0;
}