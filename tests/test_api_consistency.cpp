// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>

#define REQUIRE(expr) do { if (!(expr)) { std::cerr << "FAILED: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; return EXIT_FAILURE; } } while (0)

int main() {
    using namespace configlib;

    static_assert(version_major() == 1);
    static_assert(version_minor() == 0);
    static_assert(version_patch() == 0);
    static_assert(version_string()[0] == '1');
    REQUIRE(std::string(version_string()) == "1.0.0");

    static_assert(!std::is_default_constructible_v<ConfigView>);
    static_assert(!std::is_default_constructible_v<ConfigTransaction>);

    Value text("abc");
    Value number(42);

    REQUIRE(text.as_string().has_value());
    REQUIRE(!text.as_integer().has_value());
    REQUIRE(number.as_integer().has_value());
    REQUIRE(!number.as_string().has_value());
    REQUIRE(number.as_string_or("fallback") == "fallback");

    FactSet facts;
    facts.add_default(KeyPath("xyz.logging.level"), Value("debug"));
    facts.add_default(KeyPath("xyz.logging.color"), Value(true));

    PolicySet policy = PolicySet::default_precedence();
    auto resolved = resolve(facts, policy);
    ConfigStore store = ConfigStore::from_result(resolved, policy);
    ConfigView view = store.view(KeyPath("xyz.logging"));

    REQUIRE(view.get_string(KeyPath("level")).value_or("") == "debug");
    REQUIRE(view.get_string_or(KeyPath("missing"), "fallback") == "fallback");
    REQUIRE(view.get_boolean(KeyPath("color")).value_or(false));

    ConfigSchema schema;
    schema.path(KeyPath("xyz.logging.level")).string().required().documented_default(Value("info"));
    REQUIRE(schema.validate(resolved.config()).ok());

    return EXIT_SUCCESS;
}
