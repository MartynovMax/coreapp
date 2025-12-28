#pragma once

// -----------------------------------------------------------------------------
// Version components
// -----------------------------------------------------------------------------
#ifndef CORE_VERSION_MAJOR
#define CORE_VERSION_MAJOR 0
#endif

#ifndef CORE_VERSION_MINOR
#define CORE_VERSION_MINOR 1
#endif

#ifndef CORE_VERSION_PATCH
#define CORE_VERSION_PATCH 0
#endif

// -----------------------------------------------------------------------------
// Optional semver fields.
// -----------------------------------------------------------------------------
#ifndef CORE_VERSION_PRERELEASE
#define CORE_VERSION_PRERELEASE ""
#endif

#ifndef CORE_VERSION_METADATA
#define CORE_VERSION_METADATA ""
#endif

// -----------------------------------------------------------------------------
// Numeric representation and helpers.
// -----------------------------------------------------------------------------
#define CORE_VERSION_MAKE(major, minor, patch) \
    ((int)((major) * 10000 + (minor) * 100 + (patch)))

#define CORE_VERSION_NUMBER \
    CORE_VERSION_MAKE(CORE_VERSION_MAJOR, CORE_VERSION_MINOR, CORE_VERSION_PATCH)

#define CORE_VERSION_AT_LEAST(major, minor, patch) \
    (CORE_VERSION_NUMBER >= CORE_VERSION_MAKE((major), (minor), (patch)))
