Report directory: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223

=== RUN: git status and diff summary ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/logs/00_git_status.log
CMD: git status && echo && git log --oneline --decorate -n 12 && echo && git diff --stat
PASS: git status and diff summary

=== RUN: baseline gcc/g++ build and ctest ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/logs/10_baseline_build_test.log
CMD: rm -rf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build' && cmake -S . -B '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build' -G Ninja && cmake --build '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build' -j'20' && ctest --test-dir '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build' --output-on-failure
PASS: baseline gcc/g++ build and ctest

=== RUN: debug build and ctest ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/logs/11_debug_build_test.log
CMD: rm -rf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-debug' && cmake -S . -B '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-debug' -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-debug' -j'20' && ctest --test-dir '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-debug' --output-on-failure
PASS: debug build and ctest

=== RUN: clang build and ctest ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/logs/12_clang_build_test.log
CMD: rm -rf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-clang' && CC=clang CXX=clang++ cmake -S . -B '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-clang' -G Ninja && cmake --build '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-clang' -j'20' && ctest --test-dir '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-clang' --output-on-failure
FAIL: clang build and ctest (rc=1)

=== RUN: ASAN/UBSAN build and ctest ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/logs/20_asan_ubsan.log
CMD: rm -rf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-asan' && cmake -S . -B '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-asan' -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' -DCMAKE_SHARED_LINKER_FLAGS='-fsanitize=address,undefined' && cmake --build '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-asan' -j'20' && ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1:check_initialization_order=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-asan' --output-on-failure
PASS: ASAN/UBSAN build and ctest

=== RUN: valgrind all test executables ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/valgrind/30_valgrind_tests.log
CMD: set -e; found=0; for exe in '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build'/tests/test_*; do if [ -x "$exe" ]; then found=1; echo '--- valgrind:' "$exe"; valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=99 "$exe"; fi; done; test $found -eq 1
FAIL: valgrind all test executables (rc=1)

=== RUN: cppcheck ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/static/40_cppcheck.log
CMD: cppcheck --enable=warning,style,performance,portability --std=c++20 --suppress=missingIncludeSystem include src tests examples
PASS: cppcheck

=== RUN: clang-tidy source/tests/examples ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/static/41_clang_tidy.log
CMD: rm -rf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-tidy' && cmake -S . -B '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-tidy' -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && find src tests examples -name '*.cpp' -print0 | xargs -0 clang-tidy -p '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-tidy'
FAIL: clang-tidy source/tests/examples (rc=123)

=== RUN: scan-build static analyzer ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/static/42_scan_build.log
CMD: rm -rf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-scan' '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/static/scan-build-out' && scan-build -o '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/static/scan-build-out' cmake -S . -B '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-scan' -G Ninja && scan-build -o '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/static/scan-build-out' cmake --build '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-scan' -j'20'
PASS: scan-build static analyzer

=== RUN: gcovr coverage html/xml ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/coverage/50_gcovr.log
CMD: rm -rf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-coverage' && cmake -S . -B '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-coverage' -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='--coverage -O0 -g' -DCMAKE_CXX_FLAGS='--coverage -O0 -g' -DCMAKE_EXE_LINKER_FLAGS='--coverage' -DCMAKE_SHARED_LINKER_FLAGS='--coverage' && cmake --build '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-coverage' -j'20' && ctest --test-dir '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-coverage' --output-on-failure && gcovr -r . --html --html-details -o '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/coverage/coverage.html' --xml -o '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/coverage/coverage.xml'
PASS: gcovr coverage html/xml

=== RUN: lcov/genhtml coverage ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/coverage/51_lcov.log
CMD: rm -rf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-lcov' '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/coverage/lcov-html' && cmake -S . -B '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-lcov' -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='--coverage -O0 -g' -DCMAKE_CXX_FLAGS='--coverage -O0 -g' -DCMAKE_EXE_LINKER_FLAGS='--coverage' -DCMAKE_SHARED_LINKER_FLAGS='--coverage' && cmake --build '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-lcov' -j'20' && ctest --test-dir '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-lcov' --output-on-failure && lcov --capture --directory '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-lcov' --output-file '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/coverage/lcov.info' && genhtml '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/coverage/lcov.info' --output-directory '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/coverage/lcov-html'
PASS: lcov/genhtml coverage

=== RUN: Doxygen HTML docs ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/docs/60_doxygen_html.log
CMD: rm -rf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-docs' && cmake -S . -B '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-docs' -G Ninja -DCONFIGLIB_BUILD_DOCS=ON && cmake --build '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-docs' --target configlib_docs && test -f '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-docs/docs/html/index.html'
FAIL: Doxygen HTML docs (rc=1)

=== RUN: Doxygen LaTeX/PDF docs ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/docs/61_doxygen_pdf.log
CMD: test -d '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-docs/docs/latex' || (cmake -S . -B '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-docs' -G Ninja -DCONFIGLIB_BUILD_DOCS=ON && cmake --build '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-docs' --target configlib_docs); cd '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build-docs/docs/latex' && make && cp refman.pdf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/docs/configlib_refman.pdf'
FAIL: Doxygen LaTeX/PDF docs (rc=1)

=== RUN: install tree ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/logs/70_install.log
CMD: rm -rf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test' && cmake --install '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/builds/build' --prefix '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test' && find '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test' -maxdepth 5 -type f | sort
PASS: install tree

=== RUN: external CMake C++ consumer ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/71_cmake_consumer.log
CMD: rm -rf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/cmake-consumer' && mkdir -p '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/cmake-consumer' && cat > '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/cmake-consumer/CMakeLists.txt' <<'CONSUMER_CMAKE'
cmake_minimum_required(VERSION 3.16)
project(configlib_consumer LANGUAGES CXX)
find_package(configlib CONFIG REQUIRED)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE configlib::configlib)
CONSUMER_CMAKE
cat > '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/cmake-consumer/main.cpp' <<'CONSUMER_CPP'
#include <configlib/version.hpp>
#include <iostream>
int main() { std::cout << configlib::version_string() << "\n"; return 0; }
CONSUMER_CPP
cmake -S '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/cmake-consumer' -B '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/cmake-consumer/build' -G Ninja -DCMAKE_PREFIX_PATH='/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test'
cmake --build '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/cmake-consumer/build'
'/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/cmake-consumer/build/consumer'
FAIL: external CMake C++ consumer (rc=127)

=== RUN: pkg-config C consumer ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/72_pkgconfig_c_consumer.log
CMD: rm -rf '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/c-consumer' && mkdir -p '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/c-consumer' && cat > '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/c-consumer/main.c' <<'CONSUMER_C'
#include <configlib/configlib.h>
#include <stdio.h>
int main(void) {
    printf("%s\n", configlib_version_string());
    return 0;
}
CONSUMER_C
PKG_CONFIG_PATH='/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test/lib/pkgconfig:/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test/lib64/pkgconfig' cc '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/c-consumer/main.c' -o '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/c-consumer/consumer' $(PKG_CONFIG_PATH='/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test/lib/pkgconfig:/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test/lib64/pkgconfig' pkg-config --cflags --libs configlib)
LD_LIBRARY_PATH='/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test/lib:/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test/lib64' '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/c-consumer/consumer'
FAIL: pkg-config C consumer (rc=127)

=== RUN: Python ctypes C ABI smoke ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/80_python_ctypes.log
CMD: cat > '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/python_ctypes_smoke.py' <<'PY'
import ctypes
import pathlib
prefix = pathlib.Path('/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test')
candidates = list(prefix.glob('lib*/libconfiglib.so*'))
if not candidates:
    raise SystemExit('could not find libconfiglib.so under install-test')
lib = ctypes.CDLL(str(candidates[0]))
lib.configlib_version_string.restype = ctypes.c_char_p
print(lib.configlib_version_string().decode())
PY
python3 '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/python_ctypes_smoke.py'
FAIL: Python ctypes C ABI smoke (rc=1)

=== RUN: LuaJIT FFI C ABI smoke ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/81_luajit_ffi.log
CMD: cat > '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/luajit_ffi_smoke.lua' <<'LUA'
local ffi = require('ffi')
ffi.cdef[[
const char* configlib_version_string(void);
]]
local lib = ffi.load('configlib')
print(ffi.string(lib.configlib_version_string()))
LUA
LD_LIBRARY_PATH='/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test/lib:/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test/lib64' luajit '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/luajit_ffi_smoke.lua'
FAIL: LuaJIT FFI C ABI smoke (rc=1)

=== RUN: Rust rustc C ABI smoke ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/82_rust_ffi.log
CMD: cat > '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/rust_ffi_smoke.rs' <<'RS'
use std::ffi::CStr;
use std::os::raw::c_char;

unsafe extern "C" {
    fn configlib_version_string() -> *const c_char;
}

fn main() {
    unsafe {
        let ptr = configlib_version_string();
        assert!(!ptr.is_null());
        println!("{}", CStr::from_ptr(ptr).to_str().unwrap());
    }
}
RS
rustc '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/rust_ffi_smoke.rs' -L native='/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test/lib' -L native='/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test/lib64' -l configlib -o '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/rust_ffi_smoke'
LD_LIBRARY_PATH='/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test/lib:/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test/lib64' '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/consumers/rust_ffi_smoke'
FAIL: Rust rustc C ABI smoke (rc=127)

=== RUN: abi-dumper baseline ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/abi/90_abi_dumper.log
CMD: lib=$(find '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test' -name 'libconfiglib.so*' -type f | head -n1); test -n "$lib"; abi-dumper "$lib" -o '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/abi/configlib.abi' -lver local
FAIL: abi-dumper baseline (rc=10)

=== RUN: abidw ABI XML inspection ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/abi/91_abidw.log
CMD: lib=$(find '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/artifacts/install-test' -name 'libconfiglib.so*' -type f | head -n1); test -n "$lib"; abidw "$lib" > '/home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/abi/configlib.abixml'
PASS: abidw ABI XML inspection

=== RUN: AFL++ availability ===
LOG: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/logs/95_afl_available.log
CMD: command -v afl-fuzz && afl-fuzz -h | head -n 20 || true
PASS: AFL++ availability

============================================================
configlib release gauntlet complete
Report: /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223/SUMMARY.md
Logs:   /home/henrik/Projekter/Udvikling/_reports/configlib/configlib_release_gauntlet_20260626_193223
PASS=11 FAIL=11 SKIP=0
============================================================
