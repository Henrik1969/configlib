// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>

#include <configlib/key.hpp>
#include <configlib/source.hpp>

namespace configlib {

/// Severity of a diagnostic event.
enum class Severity {
    Info,
    Warning,
    Error
};

/// One diagnostic event emitted by loaders, resolver, schema, store, or bindings.
///
/// Diagnostic codes are intended to be stable enough for tooling. Human-readable
/// messages are convenience text and may later be localized by applications.
struct Diagnostic {
    Severity severity{Severity::Info};
    std::string code;
    std::string message;
    KeyPath key;
    Source source;
};

/// Ordered collection of diagnostics.
///
/// Diagnostics are first-class output from operations. Configuration errors are
/// reported through diagnostics rather than hidden fallback behavior.
class DiagnosticLog {
public:
    void add(Diagnostic diagnostic);
    void info(std::string code, std::string message, KeyPath key = {}, Source source = {});
    void warning(std::string code, std::string message, KeyPath key = {}, Source source = {});
    void error(std::string code, std::string message, KeyPath key = {}, Source source = {});

    [[nodiscard]] bool has_errors() const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] const std::vector<Diagnostic>& items() const;
    [[nodiscard]] std::string format() const;

private:
    std::vector<Diagnostic> items_;
};

[[nodiscard]] const char* severity_name(Severity severity);

} // namespace configlib
