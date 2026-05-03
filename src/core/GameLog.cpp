#include "core/GameLog.hpp"

#include <algorithm>
#include <utility>

namespace rl {

GameLog::GameLog(std::size_t capacity)
    : capacity_(std::max<std::size_t>(1, capacity))
{
}

void GameLog::add(std::string message)
{
    messages_.push_back(std::move(message));
    while (messages_.size() > capacity_) {
        messages_.pop_front();
    }
}

std::vector<std::string_view> GameLog::recent(std::size_t count) const
{
    const auto available = std::min(count, messages_.size());
    std::vector<std::string_view> result;
    result.reserve(available);

    const auto first = messages_.size() - available;
    for (std::size_t index = first; index < messages_.size(); ++index) {
        result.push_back(messages_[index]);
    }

    return result;
}

std::size_t GameLog::size() const noexcept
{
    return messages_.size();
}

} // namespace rl
