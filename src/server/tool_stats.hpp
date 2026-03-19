#pragma once
#ifdef MCDK_SERVER

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace mcdk {
namespace server {

using json = nlohmann::json;

static constexpr int         FLUSH_INTERVAL_SEC = 1800; // 每 30 分钟写盘一次
static constexpr const char* STATS_FILE         = "tool_stats.json";
static constexpr const char* DATAS_DIR          = "datas";

class ToolStats {
public:
    explicit ToolStats(std::filesystem::path datas_dir)
        : datas_dir_(std::move(datas_dir))
        , stats_file_(datas_dir_ / STATS_FILE) {
        std::filesystem::create_directories(datas_dir_);
        load_from_disk();
        flush_thread_ = std::thread(&ToolStats::flush_loop, this);
    }

    ~ToolStats() {
        {
            std::unique_lock<std::mutex> lk(cv_mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        if (flush_thread_.joinable()) flush_thread_.join();
        flush_to_disk();
    }

    void increment(const std::string& name) {
        {
            std::shared_lock<std::shared_mutex> rl(map_mtx_);
            auto it = counters_.find(name);
            if (it != counters_.end()) {
                it->second.fetch_add(1, std::memory_order_relaxed);
                return;
            }
        }
        {
            std::unique_lock<std::shared_mutex> wl(map_mtx_);
            auto it = counters_.find(name);
            if (it == counters_.end()) {
                counters_.emplace(name, 1ULL);
            } else {
                it->second.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    struct Snapshot {
        std::string tool;
        uint64_t    total;
    };

    std::vector<Snapshot> get_snapshot() const {
        std::shared_lock<std::shared_mutex> rl(map_mtx_);
        std::lock_guard<std::mutex>         bl(base_mtx_);
        std::vector<Snapshot> result;
        result.reserve(counters_.size());
        for (const auto& [name, atom] : counters_) {
            uint64_t mem  = atom.load(std::memory_order_relaxed);
            uint64_t base = 0;
            auto     it   = base_map_.find(name);
            if (it != base_map_.end()) base = it->second;
            result.push_back({name, base + mem});
        }
        return result;
    }

    void flush_to_disk() {
        std::shared_lock<std::shared_mutex> rl(map_mtx_);
        std::lock_guard<std::mutex>         bl(base_mtx_);
        flush_locked();
    }

private:
    std::filesystem::path datas_dir_;
    std::filesystem::path stats_file_;

    mutable std::shared_mutex                               map_mtx_;
    std::unordered_map<std::string, std::atomic<uint64_t>> counters_;

    mutable std::mutex                          base_mtx_;
    std::unordered_map<std::string, uint64_t>  base_map_;

    std::thread             flush_thread_;
    std::mutex              cv_mtx_;
    std::condition_variable cv_;
    bool                    stop_ = false;

    void load_from_disk() {
        std::lock_guard<std::mutex> bl(base_mtx_);
        if (!std::filesystem::is_regular_file(stats_file_)) return;
        try {
            std::ifstream ifs(stats_file_);
            if (!ifs.is_open()) return;
            json j;
            ifs >> j;
            if (!j.is_object()) return;
            for (auto& [k, v] : j.items()) {
                uint64_t val = 0;
                if (v.is_number_unsigned())
                    val = v.get<uint64_t>();
                else if (v.is_number_integer())
                    val = static_cast<uint64_t>(std::max(int64_t(0), v.get<int64_t>()));
                base_map_[k] = val;
                counters_.emplace(k, 0ULL);
            }
        } catch (...) {}
    }

    void flush_locked() {
        try {
            json j = json::object();
            for (auto& [name, atom] : counters_) {
                uint64_t delta    = atom.exchange(0, std::memory_order_relaxed);
                base_map_[name]  += delta;
                j[name]           = base_map_[name];
            }
            auto tmp = stats_file_;
            tmp += ".tmp";
            {
                std::ofstream ofs(tmp, std::ios::trunc);
                if (!ofs.is_open()) return;
                ofs << j.dump(2) << '\n';
            }
            std::error_code ec;
            std::filesystem::rename(tmp, stats_file_, ec);
            if (ec) {
                std::filesystem::remove(stats_file_, ec);
                ec.clear();
                std::filesystem::rename(tmp, stats_file_, ec);
            }
        } catch (...) {}
    }

    void flush_loop() {
        while (true) {
            bool should_stop = false;
            {
                std::unique_lock<std::mutex> lk(cv_mtx_);
                cv_.wait_for(lk, std::chrono::seconds(FLUSH_INTERVAL_SEC),
                             [this] { return stop_; });
                should_stop = stop_;
            }
            if (should_stop) break;
            {
                std::shared_lock<std::shared_mutex> rl(map_mtx_);
                std::lock_guard<std::mutex>         bl(base_mtx_);
                flush_locked();
            }
        }
    }
};

} // namespace server
} // namespace mcdk

#endif // MCDK_SERVER
