#include "core/GameLog.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

namespace rl {
namespace {

[[nodiscard]] std::string
flattenFragments(const std::vector<LogFragment>& fragments)
{
    std::string message;
    for (const auto& fragment : fragments) {
        message += fragment.text;
    }
    return message;
}

void trimToCapacity(std::deque<LogEntry>& entries, std::size_t capacity)
{
    while (entries.size() > capacity) {
        entries.pop_front();
    }
}

} // namespace

Color defaultLogColor(LogCategory category) noexcept
{
    switch (category) {
    case LogCategory::Combat:
        return Color{255, 150, 120, 255};
    case LogCategory::Items:
        return Color{160, 220, 255, 255};
    case LogCategory::Progression:
        return Color{255, 220, 120, 255};
    case LogCategory::System:
        return Color{150, 150, 150, 255};
    case LogCategory::General:
        return Color{190, 190, 190, 255};
    }
    return Color{190, 190, 190, 255};
}

GameLog::GameLog(std::size_t capacity)
    : capacity_(std::max<std::size_t>(1, capacity))
{
}

void GameLog::add(std::string message)
{
    add(LogCategory::General, std::move(message));
}

void GameLog::add(LogCategory category, std::string message)
{
    addFragments(category,
                 {LogFragment{std::move(message), defaultLogColor(category)}});
}

void GameLog::addFragments(LogCategory category,
                           std::vector<LogFragment> fragments)
{
    if (fragments.empty()) {
        fragments.push_back(LogFragment{{}, defaultLogColor(category)});
    }

    auto message = flattenFragments(fragments);
    entries_.push_back(LogEntry{
        std::move(message), std::move(fragments), category, nextSequence_});
    ++nextSequence_;
    trimToCapacity(entries_, capacity_);
}

void GameLog::incrementCounter(std::string id, int amount)
{
    counters_[std::move(id)] += amount;
}

void GameLog::setCounter(std::string id, int amount)
{
    counters_[std::move(id)] = amount;
}

void GameLog::replaceEntries(std::vector<LogEntry> entries)
{
    entries_.clear();
    int nextSequence = 1;
    for (auto& entry : entries) {
        if (entry.fragments.empty()) {
            entry.fragments.push_back(
                LogFragment{entry.message, defaultLogColor(entry.category)});
        }
        if (entry.message.empty()) {
            entry.message = flattenFragments(entry.fragments);
        }
        if (entry.sequence <= 0) {
            entry.sequence = nextSequence;
        }
        nextSequence = std::max(nextSequence, entry.sequence + 1);
        entries_.push_back(std::move(entry));
        trimToCapacity(entries_, capacity_);
    }
    nextSequence_ = std::max(1, nextSequence);
}

std::vector<std::string_view> GameLog::recent(std::size_t count) const
{
    const auto available = std::min(count, entries_.size());
    std::vector<std::string_view> result;
    result.reserve(available);

    const auto first = entries_.size() - available;
    for (std::size_t index = first; index < entries_.size(); ++index) {
        result.push_back(entries_[index].message);
    }

    return result;
}

std::vector<LogEntry> GameLog::recentEntries(std::size_t count) const
{
    const auto available = std::min(count, entries_.size());
    const auto first = entries_.size() - available;
    return {entries_.begin() + static_cast<std::ptrdiff_t>(first),
            entries_.end()};
}

std::vector<std::string_view> GameLog::messages() const
{
    std::vector<std::string_view> result;
    result.reserve(entries_.size());
    for (const auto& entry : entries_) {
        result.push_back(entry.message);
    }
    return result;
}

std::vector<LogEntry> GameLog::entries() const
{
    return {entries_.begin(), entries_.end()};
}

std::vector<std::pair<std::string, int>> GameLog::counters() const
{
    std::vector<std::pair<std::string, int>> result;
    result.reserve(counters_.size());
    for (const auto& counter : counters_) {
        result.push_back(counter);
    }
    return result;
}

int GameLog::counter(std::string_view id) const
{
    const auto found = counters_.find(std::string{id});
    return found == counters_.end() ? 0 : found->second;
}

std::size_t GameLog::size() const noexcept
{
    return entries_.size();
}

} // namespace rl
