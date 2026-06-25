// SPDX-License-Identifier: MIT

#include <configlib/view.hpp>

#include <set>
#include <sstream>
#include <utility>

namespace configlib {

namespace {

void write_view_entry(std::ostringstream& out, const std::string& key, const Value& value, bool secret) {
    out << key << " = ";
    if (secret) out << "<redacted>";
    else out << value.to_string();
    out << '\n';
}

} // namespace

ConfigView::ConfigView(const ConfigStore& store, KeyPath prefix)
    : store_(&store), prefix_(std::move(prefix)) {}

const KeyPath& ConfigView::prefix() const { return prefix_; }
std::string ConfigView::prefix_string() const { return prefix_.dotted(); }
bool ConfigView::empty() const { return store_ == nullptr; }

KeyPath ConfigView::qualify(const KeyPath& local_key) const {
    if (prefix_.empty()) return local_key;
    if (local_key.empty()) return prefix_;
    return KeyPath(prefix_.dotted() + "." + local_key.dotted());
}

bool ConfigView::owns_full_key(const std::string& dotted) const {
    const auto prefix = prefix_.dotted();
    if (prefix.empty()) return true;
    return dotted == prefix || dotted.rfind(prefix + ".", 0) == 0;
}

std::string ConfigView::localize(const std::string& dotted) const {
    const auto prefix = prefix_.dotted();
    if (prefix.empty()) return dotted;
    if (dotted == prefix) return {};
    if (dotted.rfind(prefix + ".", 0) == 0) return dotted.substr(prefix.size() + 1);
    return dotted;
}

bool ConfigView::contains(const KeyPath& local_key) const {
    return store_ && store_->contains(qualify(local_key));
}

std::optional<Value> ConfigView::get(const KeyPath& local_key) const {
    if (!store_) return std::nullopt;
    return store_->get(qualify(local_key));
}

std::string ConfigView::get_string(const KeyPath& local_key, std::string fallback) const {
    auto value = get(local_key);
    if (!value) return fallback;
    return value->as_string(std::move(fallback));
}

std::int64_t ConfigView::get_int(const KeyPath& local_key, std::int64_t fallback) const {
    auto value = get(local_key);
    if (!value) return fallback;
    return value->as_int(fallback);
}

bool ConfigView::get_bool(const KeyPath& local_key, bool fallback) const {
    auto value = get(local_key);
    if (!value) return fallback;
    return value->as_bool(fallback);
}

double ConfigView::get_double(const KeyPath& local_key, double fallback) const {
    auto value = get(local_key);
    if (!value) return fallback;
    return value->as_double(fallback);
}

std::string ConfigView::get_string_or(const KeyPath& local_key, std::string fallback) const {
    return get_string(local_key, std::move(fallback));
}

std::int64_t ConfigView::get_int_or(const KeyPath& local_key, std::int64_t fallback) const {
    return get_int(local_key, fallback);
}

bool ConfigView::get_bool_or(const KeyPath& local_key, bool fallback) const {
    return get_bool(local_key, fallback);
}

double ConfigView::get_double_or(const KeyPath& local_key, double fallback) const {
    return get_double(local_key, fallback);
}

std::vector<std::string> ConfigView::keys() const {
    std::set<std::string> local_keys;
    if (!store_) return {};

    for (const auto& [dotted, item] : store_->runtime_overrides_) {
        (void)item;
        if (owns_full_key(dotted)) local_keys.insert(localize(dotted));
    }
    for (const auto& [dotted, entry] : store_->base_.entries()) {
        (void)entry;
        if (owns_full_key(dotted) && !store_->runtime_erases_.contains(dotted)) local_keys.insert(localize(dotted));
    }
    for (const auto& dotted : store_->runtime_erases_) {
        if (owns_full_key(dotted)) local_keys.erase(localize(dotted));
    }

    return {local_keys.begin(), local_keys.end()};
}

std::string ConfigView::explain(const KeyPath& local_key) const {
    if (!store_) return "not found: " + qualify(local_key).dotted();
    return store_->explain(qualify(local_key));
}

std::string ConfigView::export_config(ExportMode mode) const {
    std::ostringstream out;
    std::set<std::string> emitted;
    if (!store_) return {};

    auto emit_key = [&](const KeyPath& key, const Value& value) {
        const auto dotted = key.dotted();
        if (!owns_full_key(dotted)) return;
        if (emitted.contains(dotted)) return;
        if (!store_->access_.is_exportable(key) && mode != ExportMode::Redacted) return;
        write_view_entry(out, dotted, value, store_->access_.is_secret(key));
        emitted.insert(dotted);
    };

    if (mode == ExportMode::ChangedOnly || mode == ExportMode::RuntimeChangesOnly) {
        for (const auto& [_, item] : store_->runtime_overrides_) emit_key(item.first, item.second);
        for (const auto& dotted : store_->runtime_erases_) {
            if (!owns_full_key(dotted)) continue;
            KeyPath key(dotted);
            if (store_->access_.is_exportable(key)) out << dotted << " = <erased>\n";
        }
        return out.str();
    }

    for (const auto& [_, item] : store_->runtime_overrides_) emit_key(item.first, item.second);
    for (const auto& [dotted, entry] : store_->base_.entries()) {
        if (!store_->runtime_erases_.contains(dotted)) emit_key(entry.key, entry.value);
    }
    return out.str();
}

std::string ConfigView::export_local_config(ExportMode mode) const {
    std::ostringstream out;
    std::set<std::string> emitted;
    if (!store_) return {};

    auto emit_key = [&](const KeyPath& key, const Value& value) {
        const auto dotted = key.dotted();
        if (!owns_full_key(dotted)) return;
        const auto local = localize(dotted);
        if (emitted.contains(local)) return;
        if (!store_->access_.is_exportable(key) && mode != ExportMode::Redacted) return;
        write_view_entry(out, local, value, store_->access_.is_secret(key));
        emitted.insert(local);
    };

    if (mode == ExportMode::ChangedOnly || mode == ExportMode::RuntimeChangesOnly) {
        for (const auto& [_, item] : store_->runtime_overrides_) emit_key(item.first, item.second);
        for (const auto& dotted : store_->runtime_erases_) {
            if (!owns_full_key(dotted)) continue;
            KeyPath key(dotted);
            if (store_->access_.is_exportable(key)) out << localize(dotted) << " = <erased>\n";
        }
        return out.str();
    }

    for (const auto& [_, item] : store_->runtime_overrides_) emit_key(item.first, item.second);
    for (const auto& [dotted, entry] : store_->base_.entries()) {
        if (!store_->runtime_erases_.contains(dotted)) emit_key(entry.key, entry.value);
    }
    return out.str();
}

} // namespace configlib
