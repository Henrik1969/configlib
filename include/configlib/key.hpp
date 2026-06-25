#pragma once

#include <string>
#include <vector>

namespace configlib {

class KeyPath {
public:
    KeyPath() = default;
    explicit KeyPath(std::string dotted);
    explicit KeyPath(std::vector<std::string> parts);

    [[nodiscard]] const std::vector<std::string>& parts() const;
    [[nodiscard]] const std::string& dotted() const;
    [[nodiscard]] bool empty() const;

    friend bool operator==(const KeyPath& lhs, const KeyPath& rhs);
    friend bool operator<(const KeyPath& lhs, const KeyPath& rhs);

private:
    std::vector<std::string> parts_;
    std::string dotted_;

    void rebuild_dotted();
    void rebuild_parts();
};

} // namespace configlib
