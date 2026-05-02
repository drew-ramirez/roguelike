#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <string_view>
#include <vector>

namespace rl {

class GameLog {
public:
    explicit GameLog(std::size_t capacity = 100);

    void add(std::string message);

    [[nodiscard]] std::vector<std::string_view> recent(std::size_t count) const;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::size_t capacity_ = 100;
    std::deque<std::string> messages_;
};

} // namespace rl
