#include "../runner/experiment_registry.hpp"
#include <gtest/gtest.h>

using namespace core::bench;

// Test fixture
class ExperimentRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry.Clear();
    }

    ExperimentRegistry registry;
};

// Placeholder test
TEST_F(ExperimentRegistryTest, Placeholder) {
    EXPECT_EQ(registry.Count(), 0u);
}
