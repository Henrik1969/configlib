// SPDX-License-Identifier: MIT

#pragma once

/// @file version.hpp
/// Compile-time version constants and helpers for configlib.

#define CONFIGLIB_VERSION_MAJOR 0
#define CONFIGLIB_VERSION_MINOR 13
#define CONFIGLIB_VERSION_PATCH 1
#define CONFIGLIB_VERSION_STRING "0.13.1"

namespace configlib {

/// Returns the configlib major version.
constexpr int version_major() noexcept { return CONFIGLIB_VERSION_MAJOR; }

/// Returns the configlib minor version.
constexpr int version_minor() noexcept { return CONFIGLIB_VERSION_MINOR; }

/// Returns the configlib patch version.
constexpr int version_patch() noexcept { return CONFIGLIB_VERSION_PATCH; }

/// Returns the configlib version as a static null-terminated string.
constexpr const char* version_string() noexcept { return CONFIGLIB_VERSION_STRING; }

} // namespace configlib
