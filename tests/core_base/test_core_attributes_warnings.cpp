#include <gtest/gtest.h>

#include "core/base/core_config.hpp"

namespace {

	CORE_NODISCARD int MustUseResult() { return 7; }

	CORE_DEPRECATED int OldPlusOne(int x) { return x + 1; }

	CORE_DEPRECATED_MSG("use NewPlusOne() instead") int OldPlusOneMsg(int x) {
		return x + 1;
	}

} // namespace

TEST(CoreAttributesWarnings, EmitsUsefulWarnings) {
	// Intentionally ignore nodiscard return value (should warn when warnings enabled).
	MustUseResult();

	// Intentionally call deprecated APIs (should warn).
	EXPECT_EQ(OldPlusOne(10), 11);
	EXPECT_EQ(OldPlusOneMsg(10), 11);
}
