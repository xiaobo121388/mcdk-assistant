#include "search/search_service.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    std::cout << "[test] loading knowledge base..." << std::endl;
    mcdk::SearchService svc(MCDK_DICTS_DIR, MCDK_KNOWLEDGE_DIR);
    std::cout << "[test] indexed " << svc.doc_count() << " fragments" << std::endl;
    std::cout << "[test] enter keywords to search (empty line to quit):\n" << std::endl;

    std::string line;
    while (true) {
        std::cout << ">>> ";
        if (!std::getline(std::cin, line) || line.empty()) break;

        auto results = svc.search(line);
        if (results.empty()) {
            std::cout << "(no results)\n" << std::endl;
            continue;
        }

        std::cout << "--- " << results.size() << " results ---\n";
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& r = results[i];
            std::cout << "[" << i + 1 << "] score=" << r.score
                      << "  file=" << r.fragment->file
                      << "  lines=" << r.fragment->line_start << "-" << r.fragment->line_end
                      << "\n" << r.fragment->content << "\n";
        }
        std::cout << std::endl;
    }

    return 0;
}
