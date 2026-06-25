// SPDX-License-Identifier: MIT

#include <configlib/key.hpp>

#include <sstream>

namespace configlib {

KeyPath::KeyPath(std::string dotted) : dotted_(std::move(dotted)) { rebuild_parts(); }
KeyPath::KeyPath(std::vector<std::string> parts) : parts_(std::move(parts)) { rebuild_dotted(); }

const std::vector<std::string>& KeyPath::parts() const { return parts_; }
const std::string& KeyPath::dotted() const { return dotted_; }
bool KeyPath::empty() const { return dotted_.empty(); }

void KeyPath::rebuild_dotted() {
    dotted_.clear();
    for (std::size_t i = 0; i < parts_.size(); ++i) {
        if (i != 0) dotted_ += '.';
        dotted_ += parts_[i];
    }
}

void KeyPath::rebuild_parts() {
    parts_.clear();
    std::stringstream ss(dotted_);
    std::string part;
    while (std::getline(ss, part, '.')) {
        if (!part.empty()) parts_.push_back(part);
    }
}

bool operator==(const KeyPath& lhs, const KeyPath& rhs) { return lhs.dotted_ == rhs.dotted_; }
bool operator<(const KeyPath& lhs, const KeyPath& rhs) { return lhs.dotted_ < rhs.dotted_; }

} // namespace configlib
