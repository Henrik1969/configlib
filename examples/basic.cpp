#include <configlib/configlib.hpp>

#include <iostream>

int main() {
    using namespace configlib;

    FactSet facts;
    facts.add_default(KeyPath("logging.level"), Value("info"));
    facts.add_file(KeyPath("logging.level"), Value("warn"), "./app.conf");
    facts.add_env(KeyPath("logging.level"), Value("debug"), "MYAPP_LOGGING_LEVEL");
    facts.add_cli(KeyPath("logging.level"), Value("trace"), "--log-level");
    facts.add_file(KeyPath("server.port"), Value(8080), "./app.conf");

    PolicySet policy;
    policy.require(KeyPath("logging.level"), ValueType::String);
    policy.allowed_strings(KeyPath("logging.level"), {"trace", "debug", "info", "warn", "error"});
    policy.defaulted(KeyPath("server.host"), Value("127.0.0.1"));
    policy.require(KeyPath("server.port"), ValueType::Int);
    policy.int_range(KeyPath("server.port"), 1, 65535);

    auto result = resolve(facts, policy);

    if (!result.ok()) {
        std::cerr << result.diagnostics().format();
        return 1;
    }

    std::cout << "logging.level = " << result.config().get_string(KeyPath("logging.level")) << '\n';
    std::cout << "server.host = " << result.config().get_string(KeyPath("server.host")) << '\n';
    std::cout << "server.port = " << result.config().get_int(KeyPath("server.port")) << '\n';
    std::cout << "\nExplanation:\n" << result.config().format_explanation(KeyPath("logging.level"));
}
