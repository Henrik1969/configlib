// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <iostream>

int main() {
    using namespace configlib;

    FactSet facts;
    facts.add_default(KeyPath("logging.level"), Value("info"));
    facts.add_file(KeyPath("logging.level"), Value("warn"), "./app.conf");
    facts.add_cli(KeyPath("server.port"), Value(8080), "--port");
    facts.add_default(KeyPath("auth.token"), Value("secret-token"));

    PolicySet policies;
    facts.add_default(KeyPath("logging.level"), Value("info"));
    policies.allowed_strings(KeyPath("logging.level"), {"trace", "debug", "info", "warn", "error"});
    policies.require(KeyPath("server.port"), ValueType::Int);
    policies.int_range(KeyPath("server.port"), 1, 65535);
    policies.optional(KeyPath("auth.token"), ValueType::String);

    AccessPolicy access;
    access.runtime_mutable(KeyPath("server.port"), false)
          .secret(KeyPath("auth.token"));

    auto result = resolve(facts, policies);
    auto store = ConfigStore::from_result(std::move(result), policies, access);

    std::cout << "initial logging.level: " << store.get_string_or(KeyPath("logging.level"), "") << '\n';

    auto tx = store.begin_transaction();
    tx.set(KeyPath("logging.level"), Value("trace"));
    if (!tx.commit()) {
        std::cerr << tx.diagnostics().format();
        return 1;
    }

    std::cout << "runtime logging.level: " << store.get_string_or(KeyPath("logging.level"), "") << '\n';
    std::cout << "changed only:\n" << store.export_config(ExportMode::ChangedOnly);
    std::cout << "effective export:\n" << store.export_config();

    auto denied = store.begin_transaction();
    denied.set(KeyPath("server.port"), Value(9000));
    if (!denied.commit()) {
        std::cout << "denied mutation:\n" << denied.diagnostics().format();
    }

    auto reset = store.begin_transaction();
    reset.reset_to_default(KeyPath("logging.level"));
    reset.commit();
    std::cout << "after default reset: " << store.get_string_or(KeyPath("logging.level"), "") << '\n';
}
