///Harness de diff: ruleaza un fisier LOBSTER *_message_*.csv prin OrderBook si compara,
///dupa fiecare mesaj, primele N niveluri cu fisierul *_orderbook_*.csv corespunzator.
///
///   lobster_diff <message.csv> <orderbook.csv> [optiuni]
///
///     --symbol S          simbolul book-ului (implicit: dedus din numele fisierului)
///     --levels N          adancimea (implicit: dedusa din fisierul orderbook)
///     --mode synthetic    executiile devin ordine agresive prin processAddOrder (implicit)
///     --mode direct       executiile reduc direct ordinul rezident (ocoleste matching-ul)
///     --max N             opreste dupa N mesaje (implicit: tot fisierul)
///     --print N           cate divergente sa afiseze in detaliu (implicit 10)
///     --validate-every N  apeleaza OrderBook::validate() la fiecare N mesaje (implicit 0 = niciodata)

#include "LobsterReplayer.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace {

int usage(const char* argv0) {
    std::cerr << "utilizare: " << argv0 << " <message.csv> <orderbook.csv> [--symbol S] [--levels N]\n"
              << "           [--mode synthetic|direct] [--max N] [--print N] [--validate-every N]\n";
    return 2;
}

bool parseSize(const char* text, std::size_t& out) {
    try {
        out = static_cast<std::size_t>(std::stoull(text));
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

///Deduce adancimea citind prima linie de date din fisierul orderbook
int detectLevels(const std::filesystem::path& bookFile) {
    LobsterBookReader reader(bookFile, 0);
    BookSnapshot snap;
    if (!reader.next(snap))
        throw std::runtime_error("Fisierul orderbook este gol: " + bookFile.string());
    return reader.levels();
}

} ///namespace

int main(int argc, char** argv) {
    if (argc < 3)
        return usage(argv[0]);

    const std::filesystem::path messageFile{ argv[1] };
    const std::filesystem::path bookFile{ argv[2] };

    std::string   symbol;
    int           levels{};
    ExecutionMode mode{ ExecutionMode::SyntheticAggressor };
    std::size_t   maxMessages{};
    std::size_t   maxPrint{ 10 };
    std::size_t   validateEvery{};

    for (int i = 3; i < argc; ++i) {
        const std::string_view arg{ argv[i] };
        const bool hasValue = (i + 1 < argc);

        if (arg == "--symbol" && hasValue)
            symbol = argv[++i];
        else if (arg == "--levels" && hasValue)
            levels = std::stoi(argv[++i]);
        else if (arg == "--mode" && hasValue) {
            const std::string_view value{ argv[++i] };
            if (value == "synthetic")
                mode = ExecutionMode::SyntheticAggressor;
            else if (value == "direct")
                mode = ExecutionMode::DirectReduce;
            else
                return usage(argv[0]);
        }
        else if (arg == "--max" && hasValue) {
            if (!parseSize(argv[++i], maxMessages))
                return usage(argv[0]);
        }
        else if (arg == "--print" && hasValue) {
            if (!parseSize(argv[++i], maxPrint))
                return usage(argv[0]);
        }
        else if (arg == "--validate-every" && hasValue) {
            if (!parseSize(argv[++i], validateEvery))
                return usage(argv[0]);
        }
        else
            return usage(argv[0]);
    }

    try {
        const LobsterFileInfo info = inferLobsterFileInfo(messageFile);
        if (symbol.empty())
            symbol = info.symbol.empty() ? std::string{ "UNKNOWN" } : info.symbol;
        if (levels <= 0)
            levels = detectLevels(bookFile);

        std::cout << "simbol            : " << symbol << '\n'
                  << "niveluri          : " << levels << '\n'
                  << "mod executii      : "
                  << (mode == ExecutionMode::SyntheticAggressor ? "agresor sintetic (trece prin matching)"
                                                                : "reducere directa (ocoleste matching-ul)")
                  << "\n\n";

        LobsterReplayer replayer{ symbol, levels, mode };
        const ReplayStats stats = replayer.run(messageFile, bookFile, maxMessages,
                                               maxPrint, validateEvery, std::cout);

        std::cout << "\n--------------------------------------------------\n"
                  << "mesaje citite     : " << stats.messages    << '\n'
                  << "  tip 1 add       : " << stats.byType[1]   << '\n'
                  << "  tip 2 cancel p. : " << stats.byType[2]   << '\n'
                  << "  tip 3 delete    : " << stats.byType[3]   << '\n'
                  << "  tip 4 exec vis. : " << stats.byType[4]   << '\n'
                  << "  tip 5 exec hid. : " << stats.byType[5]   << '\n'
                  << "  tip 6 cross     : " << stats.byType[6]   << '\n'
                  << "  tip 7 halt      : " << stats.byType[7]   << '\n'
                  << "aplicate pe book  : " << stats.applied     << '\n'
                  << "ignorate          : " << stats.skipped     << '\n'
                  << "comparatii        : " << stats.comparisons << '\n'
                  << "divergente        : " << stats.divergences << '\n'
                  << "erori de motor    : " << stats.engineErrors << '\n'
                  << "--------------------------------------------------\n"
                  << (stats.ok() ? "OK - book-ul reproduce exact ground truth-ul\n"
                                 : "ESUAT - vezi divergentele de mai sus\n");

        return stats.ok() ? 0 : 1;
    }
    catch (const std::exception& e) {
        std::cerr << "eroare: " << e.what() << '\n';
        return 3;
    }
}
