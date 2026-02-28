#pragma once

#include "search/bm25.h"
#include <cppjieba/Jieba.hpp>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <mutex>

namespace mcdk {

// 文档搜索服务：加载 Markdown 知识库 + jieba 分词 + BM25 搜索
class SearchService {
public:
    SearchService(const std::string& dicts_dir, const std::string& knowledge_dir)
        : jieba_(
            dicts_dir + "/jieba.dict.utf8",
            dicts_dir + "/hmm_model.utf8",
            dicts_dir + "/user.dict.utf8",
            dicts_dir + "/idf.utf8",
            dicts_dir + "/stop_words.utf8"
          )
        , knowledge_dir_(knowledge_dir)
    {
        load_stop_words(dicts_dir + "/stop_words.utf8");
        load_knowledge();
        build_index();
    }

    // 搜索接口
    std::vector<SearchResult> search(const std::string& keyword, int top_k = -1) const {
        std::vector<std::string> query_tokens;
        tokenize(keyword, query_tokens);
        return engine_.search(query_tokens, top_k);
    }

    size_t doc_count() const { return fragments_.size(); }

private:
    cppjieba::Jieba                          jieba_;
    std::string                              knowledge_dir_;
    std::vector<DocFragment>                 fragments_;
    std::vector<std::vector<std::string>>    tokenized_docs_;
    BM25Engine                               engine_;
    std::unordered_set<std::string>          stop_words_;

    void load_stop_words(const std::string& path) {
        std::ifstream ifs(path);
        if (!ifs.is_open()) return;
        std::string line;
        while (std::getline(ifs, line)) {
            if (!line.empty()) {
                // 去除可能的 \r
                if (!line.empty() && line.back() == '\r') line.pop_back();
                stop_words_.insert(line);
            }
        }
    }

    void tokenize(const std::string& text, std::vector<std::string>& tokens) const {
        std::vector<std::string> raw;
        jieba_.CutForSearch(text, raw);
        tokens.clear();
        for (auto& w : raw) {
            // 过滤空白和停用词
            if (w.empty() || w == " " || w == "\t" || w == "\n") continue;
            if (stop_words_.count(w)) continue;
            tokens.push_back(std::move(w));
        }
    }

    // 递归加载 knowledge 目录下所有 .md 文件
    void load_knowledge() {
        namespace fs = std::filesystem;
        if (!fs::exists(knowledge_dir_)) {
            std::cerr << "[mcdk] knowledge dir not found: " << knowledge_dir_ << std::endl;
            return;
        }

        for (const auto& entry : fs::recursive_directory_iterator(knowledge_dir_)) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            if (ext != ".md" && ext != ".MD") continue;

            std::string rel_path = fs::relative(entry.path(), knowledge_dir_).generic_string();
            load_markdown_file(entry.path().string(), rel_path);
        }

        std::cout << "[mcdk] loaded " << fragments_.size()
                  << " fragments from " << knowledge_dir_ << std::endl;
    }

    // 按 Markdown 标题(## / ###)分割文档为片段
    void load_markdown_file(const std::string& abs_path, const std::string& rel_path) {
        std::ifstream ifs(abs_path);
        if (!ifs.is_open()) return;

        std::string line;
        std::ostringstream current_content;
        int fragment_start = 1;
        int line_num = 0;
        bool has_content = false;

        auto flush_fragment = [&]() {
            std::string content = current_content.str();
            if (!content.empty() && has_content) {
                fragments_.push_back({
                    std::move(content),
                    rel_path,
                    fragment_start,
                    line_num
                });
            }
            current_content.str("");
            current_content.clear();
            has_content = false;
            fragment_start = line_num + 1;
        };

        while (std::getline(ifs, line)) {
            ++line_num;
            // 去除 \r
            if (!line.empty() && line.back() == '\r') line.pop_back();

            // 检测标题行 (## 或 ###)，作为分割点
            if (line.size() >= 2 && line[0] == '#') {
                // 统计 # 数量
                size_t level = 0;
                while (level < line.size() && line[level] == '#') ++level;
                if (level >= 2 && level <= 4) {
                    // 新的片段开始，先保存之前的
                    flush_fragment();
                    fragment_start = line_num;
                }
            }

            current_content << line << "\n";
            if (!line.empty()) has_content = true;
        }

        // 最后一个片段
        flush_fragment();
    }

    void build_index() {
        tokenized_docs_.resize(fragments_.size());
        for (size_t i = 0; i < fragments_.size(); ++i) {
            tokenize(fragments_[i].content, tokenized_docs_[i]);
        }
        engine_.build_index(fragments_, tokenized_docs_);
        std::cout << "[mcdk] BM25 index built, " << fragments_.size() << " documents indexed" << std::endl;
    }
};

} // namespace mcdk
