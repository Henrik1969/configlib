#pragma once

#include <string>

namespace configlib {

enum class SourceKind {
    InternalDefault,
    File,
    Environment,
    CLI,
    Runtime,
    Unknown
};

class Source {
public:
    Source();
    Source(SourceKind kind, std::string name);

    static Source internal_default(std::string name = "internal-default");
    static Source file(std::string path);
    static Source environment(std::string variable_name);
    static Source cli(std::string option_name);
    static Source runtime(std::string name = "runtime");

    [[nodiscard]] SourceKind kind() const;
    [[nodiscard]] const std::string& name() const;
    [[nodiscard]] std::string describe() const;

private:
    SourceKind kind_;
    std::string name_;
};

[[nodiscard]] const char* source_kind_name(SourceKind kind);

} // namespace configlib
