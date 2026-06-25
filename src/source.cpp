#include <configlib/source.hpp>

namespace configlib {

Source::Source() : kind_(SourceKind::Unknown), name_("unknown") {}
Source::Source(SourceKind kind, std::string name) : kind_(kind), name_(std::move(name)) {}

Source Source::internal_default(std::string name) { return {SourceKind::InternalDefault, std::move(name)}; }
Source Source::file(std::string path) { return {SourceKind::File, std::move(path)}; }
Source Source::environment(std::string variable_name) { return {SourceKind::Environment, std::move(variable_name)}; }
Source Source::cli(std::string option_name) { return {SourceKind::CLI, std::move(option_name)}; }
Source Source::runtime(std::string name) { return {SourceKind::Runtime, std::move(name)}; }

SourceKind Source::kind() const { return kind_; }
const std::string& Source::name() const { return name_; }

std::string Source::describe() const {
    return std::string(source_kind_name(kind_)) + ":" + name_;
}

const char* source_kind_name(SourceKind kind) {
    switch (kind) {
        case SourceKind::InternalDefault: return "internal-default";
        case SourceKind::File: return "file";
        case SourceKind::Environment: return "env";
        case SourceKind::CLI: return "cli";
        case SourceKind::Runtime: return "runtime";
        case SourceKind::Unknown: return "unknown";
    }
    return "unknown";
}

} // namespace configlib
