// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#define REQUIRE(expr) do { if (!(expr)) { std::cerr << "FAILED: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; return EXIT_FAILURE; } } while (0)

namespace {

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace

int main() {
    using namespace configlib;

    // Fire test: malformed direct facts must not silently enter the ruling.
    {
        FactSet facts;
        facts.add_file(KeyPath("bad..key"), Value("poison"), "hostile.conf");
        facts.add_file(KeyPath("good.key"), Value("safe"), "hostile.conf");
        auto result = resolve(facts, PolicySet{});
        REQUIRE(!result.ok());
        REQUIRE(!result.config().contains(KeyPath("bad..key")));
        REQUIRE(result.config().get_string(KeyPath("good.key")) == "safe");
        REQUIRE(contains(result.diagnostics().format(), "CONFIG_INVALID_KEY"));
    }

    // Fire test: same priority but same value is not ambiguous.
    {
        FactSet facts;
        facts.add(KeyPath("logging.level"), Value("debug"), Source::file("a.conf"), 30);
        facts.add(KeyPath("logging.level"), Value("debug"), Source::file("b.conf"), 30);
        PolicySet policy;
        policy.require(KeyPath("logging.level"), ValueType::String);
        auto result = resolve(facts, policy);
        REQUIRE(result.ok());
        REQUIRE(result.config().get_string(KeyPath("logging.level")) == "debug");
    }

    // Fire test: ambiguous competing facts complain, but still preserve diagnostics and do not crash.
    {
        FactSet facts;
        facts.add(KeyPath("mode"), Value("alpha"), Source::file("a.conf"), 40);
        facts.add(KeyPath("mode"), Value("beta"), Source::file("b.conf"), 40);
        PolicySet policy;
        policy.require(KeyPath("mode"), ValueType::String);
        auto result = resolve(facts, policy);
        REQUIRE(!result.ok());
        REQUIRE(contains(result.diagnostics().format(), "CONFIG_AMBIGUOUS_PRECEDENCE"));
    }

    // Fire test: bad file keys are rejected at loader boundary with a reason.
    {
        const auto path = std::filesystem::temp_directory_path() / "configlib_fire_bad_keys.conf";
        {
            std::ofstream out(path);
            out << "good.key = yes\n";
            out << "bad..key = nope\n";
            out << "[section]\n";
            out << ".bad = nope\n";
        }
        auto report = load_config_file(path.string(), PolicySet{});
        std::filesystem::remove(path);
        REQUIRE(!report.ok());
        REQUIRE(report.facts.for_key(KeyPath("good.key")).size() == 1);
        REQUIRE(report.facts.for_key(KeyPath("bad..key")).empty());
        REQUIRE(contains(report.diagnostics.format(), "CONFIG_FILE_BAD_KEY"));
    }

    // Fire test: invalid environment-derived keys are rejected before they become facts.
    {
        EnvironmentLoaderPolicy env;
        env.enabled().prefix("APP_").mapping_style(EnvMappingStyle::PrefixToDottedLowercase);
        std::map<std::string, std::string> environment{
            {"APP_GOOD_KEY", "value"},
            {"APP_BAD__KEY", "poison"},
            {"APP_", "empty"}
        };
        auto report = load_environment(env, PolicySet{}, environment);
        REQUIRE(!report.ok());
        REQUIRE(report.facts.for_key(KeyPath("good.key")).size() == 1);
        REQUIRE(report.facts.for_key(KeyPath("bad..key")).empty());
        REQUIRE(contains(report.diagnostics.format(), "CONFIG_ENV_BAD_KEY"));
    }

    // Fire test: malformed CLI-mapped keys produce diagnostics instead of bad facts.
    {
        CliLoaderPolicy cli;
        cli.enabled().option("--bad", KeyPath("bad..key"), ValueType::String)
                     .option("--good", KeyPath("good.key"), ValueType::String);
        std::vector<std::string> argv{"app", "--bad", "poison", "--good", "safe"};
        auto report = load_cli(cli, PolicySet{}, argv);
        REQUIRE(!report.ok());
        REQUIRE(report.facts.for_key(KeyPath("good.key")).size() == 1);
        REQUIRE(report.facts.for_key(KeyPath("bad..key")).empty());
        REQUIRE(contains(report.diagnostics.format(), "CONFIG_CLI_BAD_KEY"));
    }

    // Fire test: transaction failure leaves prior state untouched.
    {
        FactSet facts;
        facts.add_default(KeyPath("logging.level"), Value("info"));
        PolicySet policy;
        policy.allowed_strings(KeyPath("logging.level"), {"info", "debug"});
        auto result = resolve(facts, policy);
        REQUIRE(result.ok());
        auto store = ConfigStore::from_result(std::move(result), policy, AccessPolicy{});

        auto ok_tx = store.begin_transaction();
        ok_tx.set_string(KeyPath("logging.level"), "debug");
        REQUIRE(ok_tx.commit());
        REQUIRE(store.get_string(KeyPath("logging.level")) == "debug");

        auto bad_tx = store.begin_transaction();
        bad_tx.set_string(KeyPath("logging.level"), "trace");
        REQUIRE(!bad_tx.commit());
        REQUIRE(store.get_string(KeyPath("logging.level")) == "debug");
        REQUIRE(contains(bad_tx.diagnostics().format(), "CONFIG_STORE_STRING_NOT_ALLOWED"));
    }

    // Fire test: views cannot escape their prefix by using a similarly named key.
    {
        FactSet facts;
        facts.add_default(KeyPath("logging.level"), Value("debug"));
        facts.add_default(KeyPath("logging_extra.level"), Value("secret"));
        auto result = resolve(facts, PolicySet{});
        REQUIRE(result.ok());
        auto store = ConfigStore::from_result(std::move(result));
        auto view = store.view(KeyPath("logging"));
        REQUIRE(view.contains(KeyPath("level")));
        REQUIRE(view.get_string(KeyPath("level")) == "debug");
        const auto exported = view.export_config();
        REQUIRE(contains(exported, "logging.level = debug"));
        REQUIRE(!contains(exported, "logging_extra.level"));
        REQUIRE(!contains(exported, "secret"));
    }

    // Fire test: non-exportable and secret values do not leak in any export mode.
    {
        FactSet facts;
        facts.add_default(KeyPath("public.value"), Value("hello"));
        facts.add_default(KeyPath("secret.token"), Value("swordfish"));
        auto result = resolve(facts, PolicySet{});
        REQUIRE(result.ok());
        AccessPolicy access;
        access.secret(KeyPath("secret.token"));
        auto store = ConfigStore::from_result(std::move(result), PolicySet{}, access);
        auto tx = store.begin_transaction();
        tx.set_string(KeyPath("secret.token"), "runtime-secret");
        REQUIRE(tx.commit());

        for (auto mode : {ExportMode::Effective, ExportMode::EffectiveRedacted, ExportMode::ChangedOnly, ExportMode::ChangedOnlyRedacted, ExportMode::RuntimeChangesOnly, ExportMode::RuntimeChangesOnlyRedacted}) {
            const auto exported = store.export_config(mode);
            REQUIRE(!contains(exported, "secret.token"));
            REQUIRE(!contains(exported, "swordfish"));
            REQUIRE(!contains(exported, "runtime-secret"));
        }
    }

    // Fire test: simple stress load should remain deterministic and choose highest precedence.
    {
        FactSet facts;
        for (int i = 0; i < 1000; ++i) {
            facts.add(KeyPath("stress.value"), Value(static_cast<std::int64_t>(i)), Source::file("stress.conf"), 30);
        }
        facts.add_cli(KeyPath("stress.value"), Value(static_cast<std::int64_t>(4242)), "--stress-value");
        PolicySet policy;
        policy.require(KeyPath("stress.value"), ValueType::Int);
        auto result = resolve(facts, policy);
        REQUIRE(result.ok());
        REQUIRE(result.config().get_integer(KeyPath("stress.value")) == 4242);
    }

    return EXIT_SUCCESS;
}
