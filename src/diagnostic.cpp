#include <configlib/diagnostic.hpp>

#include <sstream>

namespace configlib {

void DiagnosticLog::add(Diagnostic diagnostic) { items_.push_back(std::move(diagnostic)); }

void DiagnosticLog::info(std::string code, std::string message, KeyPath key, Source source) {
    add({Severity::Info, std::move(code), std::move(message), std::move(key), std::move(source)});
}

void DiagnosticLog::warning(std::string code, std::string message, KeyPath key, Source source) {
    add({Severity::Warning, std::move(code), std::move(message), std::move(key), std::move(source)});
}

void DiagnosticLog::error(std::string code, std::string message, KeyPath key, Source source) {
    add({Severity::Error, std::move(code), std::move(message), std::move(key), std::move(source)});
}

bool DiagnosticLog::has_errors() const {
    for (const auto& item : items_) {
        if (item.severity == Severity::Error) return true;
    }
    return false;
}

bool DiagnosticLog::empty() const { return items_.empty(); }
const std::vector<Diagnostic>& DiagnosticLog::items() const { return items_; }

std::string DiagnosticLog::format() const {
    std::ostringstream out;
    for (const auto& item : items_) {
        out << severity_name(item.severity) << " " << item.code << ": " << item.message;
        if (!item.key.empty()) out << " [key=" << item.key.dotted() << "]";
        if (item.source.kind() != SourceKind::Unknown) out << " [source=" << item.source.describe() << "]";
        out << '\n';
    }
    return out.str();
}

const char* severity_name(Severity severity) {
    switch (severity) {
        case Severity::Info: return "info";
        case Severity::Warning: return "warning";
        case Severity::Error: return "error";
    }
    return "unknown";
}

} // namespace configlib
