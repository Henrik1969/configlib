#!/usr/bin/env bash
# configlib_quick_fire_retest.sh
#
# Focused local retest after first firetest findings.
# Run from configlib repo root. Writes reports outside the repo.

set -u
set -o pipefail

PROJECT_NAME="configlib"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
REPO_ROOT="$(pwd)"
REPORT_ROOT="${REPORT_ROOT:-$HOME/Projekter/Udvikling/_reports/${PROJECT_NAME}}"
REPORT_DIR="${REPORT_ROOT}/${PROJECT_NAME}_quick_fire_retest_${TIMESTAMP}"
JOBS="${JOBS:-$(nproc)}"
STRICT_INFRA="${STRICT_INFRA:-0}"

mkdir -p "$REPORT_DIR"/{logs,builds,docs,consumers,abi,valgrind,coverage,artifacts}

SUMMARY="$REPORT_DIR/SUMMARY.md"
COMMAND_LOG="$REPORT_DIR/commands.log"

pass_count=0
fail_count=0
infra_count=0
skip_count=0

have() { command -v "$1" >/dev/null 2>&1; }

log() { printf '%s\n' "$*" | tee -a "$COMMAND_LOG"; }

status_line() {
    printf '| %s | %s | %s |\n' "$1" "$2" "${3:-}" >> "$SUMMARY"
}

pass() {
    echo "PASS: $1"
    status_line "PASS" "$1" "$2"
    pass_count=$((pass_count + 1))
}

fail() {
    echo "FAIL: $1"
    status_line "FAIL" "$1" "$2"
    fail_count=$((fail_count + 1))
}

infra() {
    echo "INFRA: $1 -- $2"
    status_line "INFRA" "$1" "$2"
    infra_count=$((infra_count + 1))
    if [ "$STRICT_INFRA" = "1" ]; then
        fail_count=$((fail_count + 1))
    fi
}

skip() {
    echo "SKIP: $1 -- $2"
    status_line "SKIP" "$1" "$2"
    skip_count=$((skip_count + 1))
}

run_cmd() {
    local name="$1"
    local logfile="$2"
    local cmd="$3"

    log ""
    log "=== RUN: $name ==="
    log "LOG: $logfile"
    log "CMD: $cmd"

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
        pass "$name" "$logfile"
        return 0
    fi

    fail "$name" "$logfile"
    return "$rc"
}

run_infra_cmd() {
    local name="$1"
    local logfile="$2"
    local cmd="$3"

    log ""
    log "=== RUN/INFRA: $name ==="
    log "LOG: $logfile"
    log "CMD: $cmd"

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
        pass "$name" "$logfile"
        return 0
    fi

    infra "$name" "$logfile"
    return "$rc"
}

if [ ! -f CMakeLists.txt ] || [ ! -d include/configlib ]; then
    echo "ERROR: run from configlib repo root."
    exit 2
fi

cat > "$SUMMARY" <<EOF
# configlib quick fire retest

Generated: \`$(date -Iseconds)\`  
Host: \`$(hostname)\`  
Repo: \`$REPO_ROOT\`  
Report dir: \`$REPORT_DIR\`  
Jobs: \`$JOBS\`  
Strict infra: \`$STRICT_INFRA\`

## Purpose

Focused retest after first firetest findings:

- stable C++/C ABI version-string consumers
- install/package use from external projects
- Doxygen output discovery
- Valgrind executable discovery
- ABI dump with debug information
- Clang toolchain status classified separately from project failure

## Git snapshot

\`\`\`text
$(git status --short 2>/dev/null || true)

$(git log --oneline --decorate -n 8 2>/dev/null || true)
\`\`\`

## Results

| Status | Test | Log / detail |
|---|---|---|
EOF

log "Report directory: $REPORT_DIR"

run_cmd "git status and version files" "$REPORT_DIR/logs/00_git_status.log" \
    "git status && echo && git log --oneline --decorate -n 12 && echo && grep -R \"version_string\\|VERSION\" -n include/configlib/version.hpp include/configlib/configlib.h src/c_api.cpp CMakeLists.txt || true"

run_cmd "baseline build/test" "$REPORT_DIR/logs/10_baseline.log" \
    "rm -rf '$REPORT_DIR/builds/build' && cmake -S . -B '$REPORT_DIR/builds/build' -G Ninja && cmake --build '$REPORT_DIR/builds/build' -j'$JOBS' && ctest --test-dir '$REPORT_DIR/builds/build' --output-on-failure"

run_cmd "ASAN/UBSAN build/test" "$REPORT_DIR/logs/20_asan_ubsan.log" \
    "rm -rf '$REPORT_DIR/builds/build-asan' && cmake -S . -B '$REPORT_DIR/builds/build-asan' -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' -DCMAKE_SHARED_LINKER_FLAGS='-fsanitize=address,undefined' && cmake --build '$REPORT_DIR/builds/build-asan' -j'$JOBS' && ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1:check_initialization_order=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir '$REPORT_DIR/builds/build-asan' --output-on-failure"

if have clang && have clang++; then
    if run_infra_cmd "clang++ standard library smoke" "$REPORT_DIR/logs/30_clang_toolchain.log" \
        "cat >/tmp/configlib_clang_smoke.cpp <<'CPP'
#include <string>
#include <map>
#include <cstdint>
int main(){ std::string s = \"ok\"; std::map<int,int> m; m[1]=2; return s.empty() || m[1] != 2; }
CPP
clang++ /tmp/configlib_clang_smoke.cpp -o /tmp/configlib_clang_smoke && /tmp/configlib_clang_smoke"
    then
        run_cmd "clang build/test" "$REPORT_DIR/logs/31_clang_build.log" \
            "rm -rf '$REPORT_DIR/builds/build-clang' && CC=clang CXX=clang++ cmake -S . -B '$REPORT_DIR/builds/build-clang' -G Ninja && cmake --build '$REPORT_DIR/builds/build-clang' -j'$JOBS' && ctest --test-dir '$REPORT_DIR/builds/build-clang' --output-on-failure"
    fi
else
    skip "clang build/test" "clang/clang++ missing"
fi

run_cmd "install tree" "$REPORT_DIR/logs/40_install.log" \
    "rm -rf '$REPORT_DIR/artifacts/install-test' && cmake --install '$REPORT_DIR/builds/build' --prefix '$REPORT_DIR/artifacts/install-test' && find '$REPORT_DIR/artifacts/install-test' -maxdepth 6 -type f | sort"

run_cmd "external CMake C++ consumer version_string" "$REPORT_DIR/consumers/50_cmake_consumer.log" \
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
int main() {
    std::cout << configlib::version_string() << \"\\n\";
    return 0;
}
CONSUMER_CPP
cmake -S '$REPORT_DIR/consumers/cmake-consumer' -B '$REPORT_DIR/consumers/cmake-consumer/build' -G Ninja -DCMAKE_PREFIX_PATH='$REPORT_DIR/artifacts/install-test'
cmake --build '$REPORT_DIR/consumers/cmake-consumer/build'
LD_LIBRARY_PATH='$REPORT_DIR/artifacts/install-test/lib:$REPORT_DIR/artifacts/install-test/lib64' '$REPORT_DIR/consumers/cmake-consumer/build/consumer'"

if have pkg-config; then
    run_cmd "pkg-config C consumer configlib_version_string" "$REPORT_DIR/consumers/51_pkgconfig_c.log" \
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
    skip "pkg-config C consumer" "pkg-config missing"
fi

if have python3; then
    run_cmd "Python ctypes version_string" "$REPORT_DIR/consumers/52_python_ctypes.log" \
        "cat > '$REPORT_DIR/consumers/python_ctypes_smoke.py' <<'PY'
import ctypes
import pathlib
prefix = pathlib.Path('$REPORT_DIR/artifacts/install-test')
libs = sorted(prefix.glob('lib*/libconfiglib.so*'))
if not libs:
    raise SystemExit('no installed libconfiglib.so found')
lib = ctypes.CDLL(str(libs[0]))
lib.configlib_version_string.restype = ctypes.c_char_p
value = lib.configlib_version_string()
if not value:
    raise SystemExit('null version string')
print(value.decode())
PY
python3 '$REPORT_DIR/consumers/python_ctypes_smoke.py'"
else
    skip "Python ctypes version_string" "python3 missing"
fi

if have luajit; then
    run_cmd "LuaJIT FFI version_string" "$REPORT_DIR/consumers/53_luajit.log" \
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
    skip "LuaJIT FFI version_string" "luajit missing"
fi

if have rustc; then
    run_cmd "Rust FFI version_string" "$REPORT_DIR/consumers/54_rust.log" \
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
    skip "Rust FFI version_string" "rustc missing"
fi

if have valgrind; then
    run_cmd "Valgrind discovered test executables" "$REPORT_DIR/valgrind/60_valgrind.log" \
        "set -e
mapfile -t tests < <(find '$REPORT_DIR/builds/build' -type f -executable \\( -name 'test_*' -o -name '*test*' \\) ! -path '*/CMakeFiles/*' | sort)
printf 'Discovered %s test executables\\n' \"\${#tests[@]}\"
printf '%s\\n' \"\${tests[@]}\"
test \"\${#tests[@]}\" -gt 0
for exe in \"\${tests[@]}\"; do
    echo '--- valgrind:' \"\$exe\"
    valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=99 \"\$exe\"
done"
else
    skip "Valgrind discovered test executables" "valgrind missing"
fi

if have doxygen; then
    run_cmd "Doxygen HTML output discovery" "$REPORT_DIR/docs/70_doxygen_html.log" \
        "rm -rf '$REPORT_DIR/builds/build-docs' && cmake -S . -B '$REPORT_DIR/builds/build-docs' -G Ninja -DCONFIGLIB_BUILD_DOCS=ON && cmake --build '$REPORT_DIR/builds/build-docs' --target configlib_docs
echo 'index.html files:'
find '$REPORT_DIR/builds/build-docs' -name index.html -print | tee '$REPORT_DIR/docs/index_files.txt'
test -s '$REPORT_DIR/docs/index_files.txt'"
else
    skip "Doxygen HTML output discovery" "doxygen missing"
fi

if have doxygen && have pdflatex && have make; then
    run_cmd "Doxygen LaTeX/PDF output discovery" "$REPORT_DIR/docs/71_doxygen_pdf.log" \
        "test -d '$REPORT_DIR/builds/build-docs' || (cmake -S . -B '$REPORT_DIR/builds/build-docs' -G Ninja -DCONFIGLIB_BUILD_DOCS=ON && cmake --build '$REPORT_DIR/builds/build-docs' --target configlib_docs)
latex_dir=\$(find '$REPORT_DIR/builds/build-docs' -type d -name latex | head -n1)
echo \"latex_dir=\$latex_dir\"
test -n \"\$latex_dir\"
cd \"\$latex_dir\"
make
find . -name '*.pdf' -print
pdf=\$(find . -name '*.pdf' | head -n1)
test -n \"\$pdf\"
cp \"\$pdf\" '$REPORT_DIR/docs/configlib_refman.pdf'"
else
    skip "Doxygen LaTeX/PDF output discovery" "doxygen/pdflatex/make missing"
fi

if have abi-dumper; then
    run_cmd "ABI dump RelWithDebInfo" "$REPORT_DIR/abi/80_abi_dumper.log" \
        "rm -rf '$REPORT_DIR/builds/build-abi' && cmake -S . -B '$REPORT_DIR/builds/build-abi' -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=ON && cmake --build '$REPORT_DIR/builds/build-abi' -j'$JOBS'
lib=\$(find '$REPORT_DIR/builds/build-abi' -name 'libconfiglib.so*' -type f | head -n1)
echo \"lib=\$lib\"
test -n \"\$lib\"
abi-dumper \"\$lib\" -o '$REPORT_DIR/abi/configlib.abi' -lver local"
else
    skip "ABI dump RelWithDebInfo" "abi-dumper missing"
fi

if have abidw; then
    run_cmd "abidw RelWithDebInfo" "$REPORT_DIR/abi/81_abidw.log" \
        "test -d '$REPORT_DIR/builds/build-abi' || (cmake -S . -B '$REPORT_DIR/builds/build-abi' -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=ON && cmake --build '$REPORT_DIR/builds/build-abi' -j'$JOBS')
lib=\$(find '$REPORT_DIR/builds/build-abi' -name 'libconfiglib.so*' -type f | head -n1)
echo \"lib=\$lib\"
test -n \"\$lib\"
abidw \"\$lib\" > '$REPORT_DIR/abi/configlib.abixml'"
else
    skip "abidw RelWithDebInfo" "abidw missing"
fi

if have gcovr; then
    run_cmd "gcovr coverage quick" "$REPORT_DIR/coverage/90_gcovr.log" \
        "rm -rf '$REPORT_DIR/builds/build-coverage' && cmake -S . -B '$REPORT_DIR/builds/build-coverage' -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='--coverage -O0 -g' -DCMAKE_CXX_FLAGS='--coverage -O0 -g' -DCMAKE_EXE_LINKER_FLAGS='--coverage' -DCMAKE_SHARED_LINKER_FLAGS='--coverage' && cmake --build '$REPORT_DIR/builds/build-coverage' -j'$JOBS' && ctest --test-dir '$REPORT_DIR/builds/build-coverage' --output-on-failure && gcovr -r . --html --html-details -o '$REPORT_DIR/coverage/coverage.html' --txt -o '$REPORT_DIR/coverage/coverage.txt' && cat '$REPORT_DIR/coverage/coverage.txt'"
else
    skip "gcovr coverage quick" "gcovr missing"
fi

cat >> "$SUMMARY" <<EOF

## Totals

\`\`\`text
PASS:  $pass_count
FAIL:  $fail_count
INFRA: $infra_count
SKIP:  $skip_count
\`\`\`

## Ruling

EOF

if [ "$fail_count" -eq 0 ]; then
    cat >> "$SUMMARY" <<EOF
**PASS for project-facing quick retest.**

No project failures were recorded. INFRA rows indicate local toolchain or
test-environment issues, not automatic project defects.
EOF
else
    cat >> "$SUMMARY" <<EOF
**FAIL.**

At least one project-facing retest failed. Review the listed logs.
EOF
fi

echo
echo "============================================================"
echo "configlib quick fire retest complete"
echo "Report: $SUMMARY"
echo "Logs:   $REPORT_DIR"
echo "PASS=$pass_count FAIL=$fail_count INFRA=$infra_count SKIP=$skip_count"
echo "============================================================"

if [ "$fail_count" -ne 0 ]; then
    exit 1
fi
exit 0
