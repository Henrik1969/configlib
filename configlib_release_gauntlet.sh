#!/usr/bin/env bash
# configlib_release_gauntlet.sh
#
# Final local release-test gauntlet for configlib.
#
# Run from the root of the configlib repository.
#
# It does not install packages.
# It does not edit source files.
# It writes logs/reports outside the repo by default.
#
# Usage:
#   chmod +x configlib_release_gauntlet.sh
#   ./configlib_release_gauntlet.sh
#
# Optional:
#   REPORT_ROOT=/some/path ./configlib_release_gauntlet.sh
#   JOBS=20 ./configlib_release_gauntlet.sh
#   STRICT=1 ./configlib_release_gauntlet.sh
#
# STRICT=1 means missing/skipped tools count as failure.

set -u
set -o pipefail

PROJECT_NAME="configlib"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
REPO_ROOT="$(pwd)"
REPORT_ROOT="${REPORT_ROOT:-$HOME/Projekter/Udvikling/_reports/${PROJECT_NAME}}"
REPORT_DIR="${REPORT_ROOT}/${PROJECT_NAME}_release_gauntlet_${TIMESTAMP}"
JOBS="${JOBS:-$(nproc)}"
STRICT="${STRICT:-0}"

mkdir -p "$REPORT_DIR"/{logs,artifacts,consumers,abi,coverage,docs,valgrind,static,builds}

SUMMARY="$REPORT_DIR/SUMMARY.md"
COMMAND_LOG="$REPORT_DIR/commands.log"

pass_count=0
fail_count=0
skip_count=0

status_line() {
    local status="$1"
    local name="$2"
    local detail="${3:-}"
    printf '| %s | %s | %s |\n' "$status" "$name" "$detail" >> "$SUMMARY"
}

note() {
    printf '%s\n' "$*" | tee -a "$COMMAND_LOG"
}

have() {
    command -v "$1" >/dev/null 2>&1
}

run_shell_step() {
    local name="$1"
    local logfile="$2"
    local cmd="$3"

    note ""
    note "=== RUN: $name ==="
    note "LOG: $logfile"
    note "CMD: $cmd"

    {
        echo "### $name"
        echo "PWD: $(pwd)"
        echo "DATE: $(date -Iseconds)"
        echo "CMD: $cmd"
        echo
        bash -lc "$cmd"
    } >"$logfile" 2>&1

    local rc=$?
    if [ "$rc" -eq 0 ]; then
        echo "PASS: $name"
        status_line "PASS" "$name" "$logfile"
        pass_count=$((pass_count + 1))
        return 0
    else
        echo "FAIL: $name (rc=$rc)"
        status_line "FAIL" "$name" "$logfile"
        fail_count=$((fail_count + 1))
        return "$rc"
    fi
}

skip_step() {
    local name="$1"
    local reason="$2"
    echo "SKIP: $name -- $reason"
    status_line "SKIP" "$name" "$reason"
    skip_count=$((skip_count + 1))
    if [ "$STRICT" = "1" ]; then
        fail_count=$((fail_count + 1))
    fi
}

require_repo() {
    if [ ! -f "CMakeLists.txt" ] || [ ! -d "include/configlib" ]; then
        echo "ERROR: this does not look like the configlib repository root."
        echo "Run from /home/henrik/Projekter/Udvikling/configlib or equivalent."
        exit 2
    fi
}

init_summary() {
    cat > "$SUMMARY" <<EOF
# configlib release gauntlet report

Generated: \`$(date -Iseconds)\`  
Host: \`$(hostname)\`  
Repo: \`$REPO_ROOT\`  
Report dir: \`$REPORT_DIR\`  
Jobs: \`$JOBS\`  
Strict mode: \`$STRICT\`

## Git state

\`\`\`text
$(git status --short 2>/dev/null || true)

HEAD:
$(git log --oneline --decorate -n 5 2>/dev/null || true)
\`\`\`

## Tool snapshot

\`\`\`text
cmake:        $(cmake --version 2>/dev/null | head -n1 || echo missing)
ninja:        $(ninja --version 2>/dev/null || echo missing)
gcc:          $(gcc --version 2>/dev/null | head -n1 || echo missing)
g++:          $(g++ --version 2>/dev/null | head -n1 || echo missing)
clang:        $(clang --version 2>/dev/null | head -n1 || echo missing)
clang++:      $(clang++ --version 2>/dev/null | head -n1 || echo missing)
valgrind:     $(valgrind --version 2>/dev/null || echo missing)
cppcheck:     $(cppcheck --version 2>/dev/null || echo missing)
clang-tidy:   $(clang-tidy --version 2>/dev/null | head -n1 || echo missing)
scan-build:   $(command -v scan-build >/dev/null && echo "$(command -v scan-build)" || echo missing)
gcovr:        $(gcovr --version 2>/dev/null | head -n1 || echo missing)
lcov:         $(lcov --version 2>/dev/null | head -n1 || echo missing)
doxygen:      $(doxygen --version 2>/dev/null || echo missing)
pdflatex:     $(pdflatex --version 2>/dev/null | head -n1 || echo missing)
abi-dumper:   $(abi-dumper --version 2>/dev/null | head -n1 || echo missing)
abidiff:      $(abidiff --version 2>/dev/null | head -n1 || echo missing)
afl-fuzz:     $(command -v afl-fuzz >/dev/null && echo "$(command -v afl-fuzz)" || echo missing)
python3:      $(python3 --version 2>/dev/null || echo missing)
rustc:        $(rustc --version 2>/dev/null || echo missing)
cargo:        $(cargo --version 2>/dev/null || echo missing)
lua:          $(lua -v 2>&1 || echo missing)
luajit:       $(luajit -v 2>&1 || echo missing)
\`\`\`

## Results

| Status | Test | Log / detail |
|---|---|---|
EOF
}

finalize_summary() {
    cat >> "$SUMMARY" <<EOF

## Totals

\`\`\`text
PASS: $pass_count
FAIL: $fail_count
SKIP: $skip_count
\`\`\`

## Release ruling

EOF

    if [ "$fail_count" -eq 0 ]; then
        cat >> "$SUMMARY" <<EOF
**PASS / READY FOR RELEASE CANDIDATE REVIEW**

No gauntlet failures were recorded. Skipped checks should still be reviewed,
but they did not block this run.
EOF
    else
        cat >> "$SUMMARY" <<EOF
**FAIL / NOT READY**

At least one gauntlet step failed or strict-mode skip was counted as failure.
Review the logs listed above before release.
EOF
    fi
}

require_repo
init_summary

note "Report directory: $REPORT_DIR"

run_shell_step "git status and diff summary" "$REPORT_DIR/logs/00_git_status.log" \
    "git status && echo && git log --oneline --decorate -n 12 && echo && git diff --stat"

run_shell_step "baseline gcc/g++ build and ctest" "$REPORT_DIR/logs/10_baseline_build_test.log" \
    "rm -rf '$REPORT_DIR/builds/build' && cmake -S . -B '$REPORT_DIR/builds/build' -G Ninja && cmake --build '$REPORT_DIR/builds/build' -j'$JOBS' && ctest --test-dir '$REPORT_DIR/builds/build' --output-on-failure"

run_shell_step "debug build and ctest" "$REPORT_DIR/logs/11_debug_build_test.log" \
    "rm -rf '$REPORT_DIR/builds/build-debug' && cmake -S . -B '$REPORT_DIR/builds/build-debug' -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build '$REPORT_DIR/builds/build-debug' -j'$JOBS' && ctest --test-dir '$REPORT_DIR/builds/build-debug' --output-on-failure"

if have clang && have clang++; then
    run_shell_step "clang build and ctest" "$REPORT_DIR/logs/12_clang_build_test.log" \
        "rm -rf '$REPORT_DIR/builds/build-clang' && CC=clang CXX=clang++ cmake -S . -B '$REPORT_DIR/builds/build-clang' -G Ninja && cmake --build '$REPORT_DIR/builds/build-clang' -j'$JOBS' && ctest --test-dir '$REPORT_DIR/builds/build-clang' --output-on-failure"
else
    skip_step "clang build and ctest" "clang/clang++ missing"
fi

run_shell_step "ASAN/UBSAN build and ctest" "$REPORT_DIR/logs/20_asan_ubsan.log" \
    "rm -rf '$REPORT_DIR/builds/build-asan' && cmake -S . -B '$REPORT_DIR/builds/build-asan' -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' -DCMAKE_SHARED_LINKER_FLAGS='-fsanitize=address,undefined' && cmake --build '$REPORT_DIR/builds/build-asan' -j'$JOBS' && ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1:check_initialization_order=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir '$REPORT_DIR/builds/build-asan' --output-on-failure"

if have valgrind; then
    run_shell_step "valgrind all test executables" "$REPORT_DIR/valgrind/30_valgrind_tests.log" \
        "set -e; found=0; for exe in '$REPORT_DIR/builds/build'/tests/test_*; do if [ -x \"\$exe\" ]; then found=1; echo '--- valgrind:' \"\$exe\"; valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=99 \"\$exe\"; fi; done; test \$found -eq 1"
else
    skip_step "valgrind all test executables" "valgrind missing"
fi

if have cppcheck; then
    run_shell_step "cppcheck" "$REPORT_DIR/static/40_cppcheck.log" \
        "cppcheck --enable=warning,style,performance,portability --std=c++20 --suppress=missingIncludeSystem include src tests examples"
else
    skip_step "cppcheck" "cppcheck missing"
fi

if have clang-tidy; then
    run_shell_step "clang-tidy source/tests/examples" "$REPORT_DIR/static/41_clang_tidy.log" \
        "rm -rf '$REPORT_DIR/builds/build-tidy' && cmake -S . -B '$REPORT_DIR/builds/build-tidy' -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && find src tests examples -name '*.cpp' -print0 | xargs -0 clang-tidy -p '$REPORT_DIR/builds/build-tidy'"
else
    skip_step "clang-tidy source/tests/examples" "clang-tidy missing"
fi

if have scan-build; then
    run_shell_step "scan-build static analyzer" "$REPORT_DIR/static/42_scan_build.log" \
        "rm -rf '$REPORT_DIR/builds/build-scan' '$REPORT_DIR/static/scan-build-out' && scan-build -o '$REPORT_DIR/static/scan-build-out' cmake -S . -B '$REPORT_DIR/builds/build-scan' -G Ninja && scan-build -o '$REPORT_DIR/static/scan-build-out' cmake --build '$REPORT_DIR/builds/build-scan' -j'$JOBS'"
else
    skip_step "scan-build static analyzer" "scan-build missing"
fi

if have gcovr; then
    run_shell_step "gcovr coverage html/xml" "$REPORT_DIR/coverage/50_gcovr.log" \
        "rm -rf '$REPORT_DIR/builds/build-coverage' && cmake -S . -B '$REPORT_DIR/builds/build-coverage' -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='--coverage -O0 -g' -DCMAKE_CXX_FLAGS='--coverage -O0 -g' -DCMAKE_EXE_LINKER_FLAGS='--coverage' -DCMAKE_SHARED_LINKER_FLAGS='--coverage' && cmake --build '$REPORT_DIR/builds/build-coverage' -j'$JOBS' && ctest --test-dir '$REPORT_DIR/builds/build-coverage' --output-on-failure && gcovr -r . --html --html-details -o '$REPORT_DIR/coverage/coverage.html' --xml -o '$REPORT_DIR/coverage/coverage.xml'"
else
    skip_step "gcovr coverage html/xml" "gcovr missing"
fi

if have lcov && have genhtml; then
    run_shell_step "lcov/genhtml coverage" "$REPORT_DIR/coverage/51_lcov.log" \
        "rm -rf '$REPORT_DIR/builds/build-lcov' '$REPORT_DIR/coverage/lcov-html' && cmake -S . -B '$REPORT_DIR/builds/build-lcov' -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='--coverage -O0 -g' -DCMAKE_CXX_FLAGS='--coverage -O0 -g' -DCMAKE_EXE_LINKER_FLAGS='--coverage' -DCMAKE_SHARED_LINKER_FLAGS='--coverage' && cmake --build '$REPORT_DIR/builds/build-lcov' -j'$JOBS' && ctest --test-dir '$REPORT_DIR/builds/build-lcov' --output-on-failure && lcov --capture --directory '$REPORT_DIR/builds/build-lcov' --output-file '$REPORT_DIR/coverage/lcov.info' && genhtml '$REPORT_DIR/coverage/lcov.info' --output-directory '$REPORT_DIR/coverage/lcov-html'"
else
    skip_step "lcov/genhtml coverage" "lcov or genhtml missing"
fi

if have doxygen; then
    run_shell_step "Doxygen HTML docs" "$REPORT_DIR/docs/60_doxygen_html.log" \
        "rm -rf '$REPORT_DIR/builds/build-docs' && cmake -S . -B '$REPORT_DIR/builds/build-docs' -G Ninja -DCONFIGLIB_BUILD_DOCS=ON && cmake --build '$REPORT_DIR/builds/build-docs' --target configlib_docs && test -f '$REPORT_DIR/builds/build-docs/docs/html/index.html'"
else
    skip_step "Doxygen HTML docs" "doxygen missing"
fi

if have doxygen && have pdflatex && have make; then
    run_shell_step "Doxygen LaTeX/PDF docs" "$REPORT_DIR/docs/61_doxygen_pdf.log" \
        "test -d '$REPORT_DIR/builds/build-docs/docs/latex' || (cmake -S . -B '$REPORT_DIR/builds/build-docs' -G Ninja -DCONFIGLIB_BUILD_DOCS=ON && cmake --build '$REPORT_DIR/builds/build-docs' --target configlib_docs); cd '$REPORT_DIR/builds/build-docs/docs/latex' && make && cp refman.pdf '$REPORT_DIR/docs/configlib_refman.pdf'"
else
    skip_step "Doxygen LaTeX/PDF docs" "doxygen/pdflatex/make missing"
fi

run_shell_step "install tree" "$REPORT_DIR/logs/70_install.log" \
    "rm -rf '$REPORT_DIR/artifacts/install-test' && cmake --install '$REPORT_DIR/builds/build' --prefix '$REPORT_DIR/artifacts/install-test' && find '$REPORT_DIR/artifacts/install-test' -maxdepth 5 -type f | sort"

run_shell_step "external CMake C++ consumer" "$REPORT_DIR/consumers/71_cmake_consumer.log" \
    "rm -rf '$REPORT_DIR/consumers/cmake-consumer' && mkdir -p '$REPORT_DIR/consumers/cmake-consumer' && cat > '$REPORT_DIR/consumers/cmake-consumer/CMakeLists.txt' <<'CONSUMER_CMAKE'
cmake_minimum_required(VERSION 3.16)
project(configlib_consumer LANGUAGES CXX)
find_package(configlib CONFIG REQUIRED)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE configlib::configlib)
CONSUMER_CMAKE
cat > '$REPORT_DIR/consumers/cmake-consumer/main.cpp' <<'CONSUMER_CPP'
#include <configlib/version.hpp>
#include <iostream>
int main() { std::cout << configlib::version_string() << \"\\n\"; return 0; }
CONSUMER_CPP
cmake -S '$REPORT_DIR/consumers/cmake-consumer' -B '$REPORT_DIR/consumers/cmake-consumer/build' -G Ninja -DCMAKE_PREFIX_PATH='$REPORT_DIR/artifacts/install-test'
cmake --build '$REPORT_DIR/consumers/cmake-consumer/build'
'$REPORT_DIR/consumers/cmake-consumer/build/consumer'"

if have pkg-config; then
    run_shell_step "pkg-config C consumer" "$REPORT_DIR/consumers/72_pkgconfig_c_consumer.log" \
        "rm -rf '$REPORT_DIR/consumers/c-consumer' && mkdir -p '$REPORT_DIR/consumers/c-consumer' && cat > '$REPORT_DIR/consumers/c-consumer/main.c' <<'CONSUMER_C'
#include <configlib/configlib.h>
#include <stdio.h>
int main(void) {
    printf(\"%s\\n\", configlib_version_string());
    return 0;
}
CONSUMER_C
PKG_CONFIG_PATH='$REPORT_DIR/artifacts/install-test/lib/pkgconfig:$REPORT_DIR/artifacts/install-test/lib64/pkgconfig' cc '$REPORT_DIR/consumers/c-consumer/main.c' -o '$REPORT_DIR/consumers/c-consumer/consumer' \$(PKG_CONFIG_PATH='$REPORT_DIR/artifacts/install-test/lib/pkgconfig:$REPORT_DIR/artifacts/install-test/lib64/pkgconfig' pkg-config --cflags --libs configlib)
LD_LIBRARY_PATH='$REPORT_DIR/artifacts/install-test/lib:$REPORT_DIR/artifacts/install-test/lib64' '$REPORT_DIR/consumers/c-consumer/consumer'"
else
    skip_step "pkg-config C consumer" "pkg-config missing"
fi

if have python3; then
    run_shell_step "Python ctypes C ABI smoke" "$REPORT_DIR/consumers/80_python_ctypes.log" \
        "cat > '$REPORT_DIR/consumers/python_ctypes_smoke.py' <<'PY'
import ctypes
import pathlib
prefix = pathlib.Path('$REPORT_DIR/artifacts/install-test')
candidates = list(prefix.glob('lib*/libconfiglib.so*'))
if not candidates:
    raise SystemExit('could not find libconfiglib.so under install-test')
lib = ctypes.CDLL(str(candidates[0]))
lib.configlib_version_string.restype = ctypes.c_char_p
print(lib.configlib_version_string().decode())
PY
python3 '$REPORT_DIR/consumers/python_ctypes_smoke.py'"
else
    skip_step "Python ctypes C ABI smoke" "python3 missing"
fi

if have luajit; then
    run_shell_step "LuaJIT FFI C ABI smoke" "$REPORT_DIR/consumers/81_luajit_ffi.log" \
        "cat > '$REPORT_DIR/consumers/luajit_ffi_smoke.lua' <<'LUA'
local ffi = require('ffi')
ffi.cdef[[
const char* configlib_version_string(void);
]]
local lib = ffi.load('configlib')
print(ffi.string(lib.configlib_version_string()))
LUA
LD_LIBRARY_PATH='$REPORT_DIR/artifacts/install-test/lib:$REPORT_DIR/artifacts/install-test/lib64' luajit '$REPORT_DIR/consumers/luajit_ffi_smoke.lua'"
else
    skip_step "LuaJIT FFI C ABI smoke" "luajit missing"
fi

if have rustc; then
    run_shell_step "Rust rustc C ABI smoke" "$REPORT_DIR/consumers/82_rust_ffi.log" \
        "cat > '$REPORT_DIR/consumers/rust_ffi_smoke.rs' <<'RS'
use std::ffi::CStr;
use std::os::raw::c_char;

unsafe extern \"C\" {
    fn configlib_version_string() -> *const c_char;
}

fn main() {
    unsafe {
        let ptr = configlib_version_string();
        assert!(!ptr.is_null());
        println!(\"{}\", CStr::from_ptr(ptr).to_str().unwrap());
    }
}
RS
rustc '$REPORT_DIR/consumers/rust_ffi_smoke.rs' -L native='$REPORT_DIR/artifacts/install-test/lib' -L native='$REPORT_DIR/artifacts/install-test/lib64' -l configlib -o '$REPORT_DIR/consumers/rust_ffi_smoke'
LD_LIBRARY_PATH='$REPORT_DIR/artifacts/install-test/lib:$REPORT_DIR/artifacts/install-test/lib64' '$REPORT_DIR/consumers/rust_ffi_smoke'"
else
    skip_step "Rust rustc C ABI smoke" "rustc missing"
fi

if have abi-dumper; then
    run_shell_step "abi-dumper baseline" "$REPORT_DIR/abi/90_abi_dumper.log" \
        "lib=\$(find '$REPORT_DIR/artifacts/install-test' -name 'libconfiglib.so*' -type f | head -n1); test -n \"\$lib\"; abi-dumper \"\$lib\" -o '$REPORT_DIR/abi/configlib.abi' -lver local"
else
    skip_step "abi-dumper baseline" "abi-dumper missing"
fi

if have abidw; then
    run_shell_step "abidw ABI XML inspection" "$REPORT_DIR/abi/91_abidw.log" \
        "lib=\$(find '$REPORT_DIR/artifacts/install-test' -name 'libconfiglib.so*' -type f | head -n1); test -n \"\$lib\"; abidw \"\$lib\" > '$REPORT_DIR/abi/configlib.abixml'"
else
    skip_step "abidw ABI XML inspection" "abidw missing"
fi

if have afl-fuzz; then
    run_shell_step "AFL++ availability" "$REPORT_DIR/logs/95_afl_available.log" \
        "command -v afl-fuzz && afl-fuzz -h | head -n 20 || true"
else
    skip_step "AFL++ availability" "afl-fuzz missing"
fi

finalize_summary

echo
echo "============================================================"
echo "configlib release gauntlet complete"
echo "Report: $SUMMARY"
echo "Logs:   $REPORT_DIR"
echo "PASS=$pass_count FAIL=$fail_count SKIP=$skip_count"
echo "============================================================"

if [ "$fail_count" -ne 0 ]; then
    exit 1
fi
exit 0
