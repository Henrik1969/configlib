// SPDX-License-Identifier: MIT

#pragma once

#include <string>

namespace configlib {

/// Origin category for a configuration fact or resolved value.
enum class SourceKind {
    InternalDefault,
    File,
    Environment,
    CLI,
    Runtime,
    Unknown
};

/// Provenance information for a configuration fact or chosen value.
///
/// A source records both a broad kind and a human-readable name, such as
/// `file:~/.config/app.conf`, `env:APP_LOG_LEVEL`, or `cli:--log-level`.
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

/// Returns a stable human-readable name for a `SourceKind`.
[[nodiscard]] const char* source_kind_name(SourceKind kind);

} // namespace configlib
