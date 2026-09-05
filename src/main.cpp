#include "FairPriceStrategy.hpp"
#include "Simulator.hpp"
#include <iostream>
#include <iomanip>

int main() {
    const std::filesystem::path eventPath{"../data/new_historical_events.txt"};

    FairPriceStrategy strategy{{
        -0.482072524654,
        8.509332824319,
        6.0,
        1.0,
        6,
    }};

    try {
        constexpr Price commissionPerShare{1};///in centi, per actiune
        Simulator S(std::make_unique<FairPriceStrategy>(strategy), 1'000'000, 21'000'000'000, commissionPerShare);
        S.loadHistoricalEvents(eventPath);
        S.scheduleTimer();
        S.run();

        const auto& books = S.getOrderBooks();
        const Symbol symbol = books.empty() ? Symbol{} : books.begin()->first;

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "input = " << eventPath << '\n';
        std::cout << "rows = " << S.getHistoricalRowCount() << '\n';
        std::cout << "trades = " << S.getFillsRecord().size() << '\n';
        std::cout << "finalPosition = " << S.getFinalPosition(symbol) << '\n';
        std::cout << "finalMark = " << (books.empty() ? "n/a" : fmtPrice(S.getOrderBook(symbol).getMarkPrice())) << '\n';
        std::cout << "finalPnlTicks = " << S.getPortfolio().getPnL(books) << '\n';
        std::cout << "minEquityTicks = " << S.getMinEquity() << '\n';
        std::cout << "maxEquityTicks = " << S.getMaxEquity() << '\n';
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
