// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <iostream>

int main() {
    using namespace configlib;

    FactSet facts;
    facts.add_default(KeyPath("flowmini.lex.strict_utf8"), Value(true));
    facts.add_default(KeyPath("flowmini.parser.recover"), Value(false));
    facts.add_default(KeyPath("flowmini.diagnostics.color"), Value(true));
    facts.add_cli(KeyPath("flowmini.diagnostics.format"), Value("human"), "--diagnostics");

    PolicySet policies;
    policies.optional(KeyPath("flowmini.lex.strict_utf8"), ValueType::Bool);
    policies.optional(KeyPath("flowmini.parser.recover"), ValueType::Bool);
    policies.optional(KeyPath("flowmini.diagnostics.color"), ValueType::Bool);
    policies.optional(KeyPath("flowmini.diagnostics.format"), ValueType::String);

    auto result = resolve(facts, policies);
    if (!result.ok()) {
        std::cerr << result.diagnostics().format();
        return 1;
    }

    auto store = ConfigStore::from_result(std::move(result), policies);
    auto lexer = store.view(KeyPath("flowmini.lex"));
    auto diagnostics = store.view(KeyPath("flowmini.diagnostics"));

    std::cout << "lexer.strict_utf8 = " << (lexer.get_bool_or(KeyPath("strict_utf8"), true) ? "true" : "false") << '\n';
    std::cout << "diagnostics.format = " << diagnostics.get_string_or(KeyPath("format"), "human") << '\n';

    std::cout << "\nDiagnostic subtree as local export:\n";
    std::cout << diagnostics.export_local_config();

    std::cout << "\nWhy diagnostics.format?\n";
    std::cout << diagnostics.explain(KeyPath("format"));

    return 0;
}
