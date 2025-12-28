#include "core/base/core_api.hpp"

// Consumer-side: declarations only. No definitions and no gtest.
// This file is compiled with CORE_BUILD_SHARED=1 (and without CORE_BUILD_CORE_LIBRARY).

struct CORE_API PublicType {
	PublicType();
	int value;
};

CORE_API int PublicFunction(int x);

// Intentionally no code and no tests: compile-only coverage.
