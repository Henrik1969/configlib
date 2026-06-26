# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.22)

project(configlib
    VERSION 0.8.0
    DESCRIPTION "Policy and fact driven configuration resolution library"
    LANGUAGES C CXX
)

option(CONFIGLIB_BUILD_SHARED "Build shared library" ON)
option(CONFIGLIB_BUILD_STATIC "Build static library" ON)
option(CONFIGLIB_BUILD_EXAMPLES "Build examples" ON)
option(CONFIGLIB_BUILD_TESTS "Build tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CONFIGLIB_PUBLIC_HEADERS
    include/configlib/configlib.hpp
    include/configlib/configlib.h
    include/configlib/binding.hpp
    include/configlib/diagnostic.hpp
    include/configlib/fact.hpp
    include/configlib/key.hpp
    include/configlib/loaders.hpp
    include/configlib/policy.hpp
    include/configlib/result.hpp
    include/configlib/schema.hpp
    include/configlib/source.hpp
    include/configlib/store.hpp
    include/configlib/value.hpp
    include/configlib/view.hpp
    include/configlib/version.hpp
)

set(CONFIGLIB_SOURCES
    src/c_api.cpp
    src/diagnostic.cpp
    src/fact.cpp
    src/key.cpp
    src/loaders.cpp
    src/policy.cpp
    src/resolver.cpp
    src/result.cpp
    src/schema.cpp
    src/source.cpp
    src/store.cpp
    src/value.cpp
    src/view.cpp
)

add_library(configlib_objects OBJECT ${CONFIGLIB_SOURCES} ${CONFIGLIB_PUBLIC_HEADERS})
target_include_directories(configlib_objects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(configlib_objects PUBLIC cxx_std_20)
target_compile_options(configlib_objects PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)
set_target_properties(configlib_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(CONFIGLIB_BUILD_SHARED)
    add_library(configlib_shared SHARED $<TARGET_OBJECTS:configlib_objects>)
    target_include_directories(configlib_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set_target_properties(configlib_shared PROPERTIES
        OUTPUT_NAME configlib
        VERSION ${PROJECT_VERSION}
        SOVERSION 0
    )
endif()

if(CONFIGLIB_BUILD_STATIC)
    add_library(configlib_static STATIC $<TARGET_OBJECTS:configlib_objects>)
    target_include_directories(configlib_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set_target_properties(configlib_static PROPERTIES OUTPUT_NAME configlib)
endif()

set(CONFIGLIB_LINK_TARGET configlib_objects)
if(CONFIGLIB_BUILD_STATIC)
    set(CONFIGLIB_LINK_TARGET configlib_static)
elseif(CONFIGLIB_BUILD_SHARED)
    set(CONFIGLIB_LINK_TARGET configlib_shared)
endif()

if(CONFIGLIB_BUILD_EXAMPLES)
    add_executable(configlib_basic_cpp examples/basic.cpp)
    target_link_libraries(configlib_basic_cpp PRIVATE ${CONFIGLIB_LINK_TARGET})

    add_executable(configlib_loaders_cpp examples/loaders.cpp)
    target_link_libraries(configlib_loaders_cpp PRIVATE ${CONFIGLIB_LINK_TARGET})

    add_executable(configlib_files_cpp examples/files.cpp)
    target_link_libraries(configlib_files_cpp PRIVATE ${CONFIGLIB_LINK_TARGET})

    add_executable(configlib_store_cpp examples/store.cpp)
    target_link_libraries(configlib_store_cpp PRIVATE ${CONFIGLIB_LINK_TARGET})

    add_executable(configlib_views_cpp examples/views.cpp)
    target_link_libraries(configlib_views_cpp PRIVATE ${CONFIGLIB_LINK_TARGET})

    add_executable(configlib_bindings_cpp examples/bindings.cpp)
    target_link_libraries(configlib_bindings_cpp PRIVATE ${CONFIGLIB_LINK_TARGET})

    add_executable(configlib_schema_cpp examples/schema.cpp)
    target_link_libraries(configlib_schema_cpp PRIVATE ${CONFIGLIB_LINK_TARGET})

    add_executable(configlib_basic_c examples/c_basic.c)
    target_link_libraries(configlib_basic_c PRIVATE ${CONFIGLIB_LINK_TARGET})
    set_target_properties(configlib_basic_c PROPERTIES LINKER_LANGUAGE CXX)
endif()

if(CONFIGLIB_BUILD_TESTS)
    enable_testing()

    add_executable(configlib_tests_cpp tests/test_configlib.cpp)
    target_link_libraries(configlib_tests_cpp PRIVATE ${CONFIGLIB_LINK_TARGET})
    add_test(NAME configlib_tests_cpp COMMAND configlib_tests_cpp)

    add_executable(configlib_tests_loaders tests/test_loaders.cpp)
    target_link_libraries(configlib_tests_loaders PRIVATE ${CONFIGLIB_LINK_TARGET})
    add_test(NAME configlib_tests_loaders COMMAND configlib_tests_loaders)

    add_executable(configlib_tests_files tests/test_files.cpp)
    target_link_libraries(configlib_tests_files PRIVATE ${CONFIGLIB_LINK_TARGET})
    add_test(NAME configlib_tests_files COMMAND configlib_tests_files)

    add_executable(configlib_tests_store tests/test_store.cpp)
    target_link_libraries(configlib_tests_store PRIVATE ${CONFIGLIB_LINK_TARGET})
    add_test(NAME configlib_tests_store COMMAND configlib_tests_store)

    add_executable(configlib_tests_views tests/test_views.cpp)
    target_link_libraries(configlib_tests_views PRIVATE ${CONFIGLIB_LINK_TARGET})
    add_test(NAME configlib_tests_views COMMAND configlib_tests_views)

    add_executable(configlib_tests_bindings tests/test_bindings.cpp)
    target_link_libraries(configlib_tests_bindings PRIVATE ${CONFIGLIB_LINK_TARGET})
    add_test(NAME configlib_tests_bindings COMMAND configlib_tests_bindings)

    add_executable(configlib_tests_schema tests/test_schema.cpp)
    target_link_libraries(configlib_tests_schema PRIVATE ${CONFIGLIB_LINK_TARGET})
    add_test(NAME configlib_tests_schema COMMAND configlib_tests_schema)

    add_executable(configlib_tests_api_consistency tests/test_api_consistency.cpp)
    target_link_libraries(configlib_tests_api_consistency PRIVATE ${CONFIGLIB_LINK_TARGET})
    add_test(NAME configlib_tests_api_consistency COMMAND configlib_tests_api_consistency)

    add_executable(configlib_tests_c tests/test_c_api.c)
    target_link_libraries(configlib_tests_c PRIVATE ${CONFIGLIB_LINK_TARGET})
    set_target_properties(configlib_tests_c PROPERTIES LINKER_LANGUAGE CXX)
    add_test(NAME configlib_tests_c COMMAND configlib_tests_c)
endif()
