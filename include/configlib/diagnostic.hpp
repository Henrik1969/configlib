// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>

#include <configlib/key.hpp>
#include <configlib/source.hpp>

namespace configlib {

enum class Severity {
    Info,
    Warning,
    Error
};

struct Diagnostic {
    Severity severity{Severity::Info};
    std::string code;
    std::string message;
    KeyPath key;
    Source source;
};

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
