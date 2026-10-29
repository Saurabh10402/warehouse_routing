#pragma once

namespace ealr {

/** Returns the stable display name used in reports and logs. */
const char* frameworkName();

/** Returns the version assigned by the top-level CMake project. */
const char* frameworkVersion();

}  // namespace ealr
