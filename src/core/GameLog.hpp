#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "render/Color.hpp"

namespace rl {

enum class LogCategory { General, Combat, Items, Progression, System };

[[nodiscard]] Color defaultLogColor(LogCategory category) noexcept;

struct LogFragment {
    std::string text;
    Color foreground = defaultLogColor(LogCategory::General);
};

struct LogEntry {
    std::string message;
    std::vector<LogFragment> fragments;
    LogCategory category = LogCategory::General;
    int sequence = 0;
};

class GameLog {
  public:
    explicit GameLog(std::size_t capacity = 100);

    void add(std::string message);
    void add(LogCategory category, std::string message);
    void addFragments(LogCategory category, std::vector<LogFragment> fragments);
    void incrementCounter(std::string id, int amount = 1);
    void setCounter(std::string id, int amount);
    void replaceEntries(std::vector<LogEntry> entries);

    [[nodiscard]] std::vector<std::string_view> recent(std::size_t count) const;
    [[nodiscard]] std::vector<LogEntry> recentEntries(std::size_t count) const;
    [[nodiscard]] std::vector<std::string_view> messages() const;
    [[nodiscard]] std::vector<LogEntry> entries() const;
    [[nodiscard]] std::vector<std::pair<std::string, int>> counters() const;
    [[nodiscard]] int counter(std::string_view id) const;
    [[nodiscard]] std::size_t size() const noexcept;

  private:
    std::size_t capacity_ = 100;
    int nextSequence_ = 1;
    std::deque<LogEntry> entries_;
    std::unordered_map<std::string, int> counters_;
};

} // namespace rl
