#include "../runner/experiment_registry.hpp"
#include "core/base/core_types.hpp"
#include <gtest/gtest.h>

using namespace core;
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

// Test experiment registration
TEST_F(ExperimentRegistryTest, RegisterExperiment) {
    ExperimentDescriptor desc;
    desc.name = "test_exp";
    desc.category = "test";
    desc.allocatorName = "test_alloc";
    desc.description = "Test description";
    desc.factory = nullptr;

    registry.Register(desc);

    EXPECT_EQ(registry.Count(), 1u);
}

// Test get experiment by name
TEST_F(ExperimentRegistryTest, FindByName) {
    ExperimentDescriptor desc;
    desc.name = "my_experiment";
    desc.category = "test";
    desc.allocatorName = "alloc";
    desc.description = "desc";
    desc.factory = nullptr;

    registry.Register(desc);

    const ExperimentDescriptor* found = registry.Find("my_experiment");
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->name, "my_experiment");

    const ExperimentDescriptor* notFound = registry.Find("nonexistent");
    EXPECT_EQ(notFound, nullptr);
}

// Test list all experiments
TEST_F(ExperimentRegistryTest, GetAll) {
    ExperimentDescriptor desc1;
    desc1.name = "exp1";
    desc1.category = "cat1";
    desc1.allocatorName = "alloc1";
    desc1.description = "desc1";
    desc1.factory = nullptr;

    ExperimentDescriptor desc2;
    desc2.name = "exp2";
    desc2.category = "cat2";
    desc2.allocatorName = "alloc2";
    desc2.description = "desc2";
    desc2.factory = nullptr;

    registry.Register(desc1);
    registry.Register(desc2);

    u32 count = 0;
    const ExperimentDescriptor* experiments = registry.GetAll(count);

    ASSERT_EQ(count, 2u);
    ASSERT_NE(experiments, nullptr);
    EXPECT_STREQ(experiments[0].name, "exp1");
    EXPECT_STREQ(experiments[1].name, "exp2");
}

// Test duplicate registration
TEST_F(ExperimentRegistryTest, DuplicateRegistration) {
    ExperimentDescriptor desc;
    desc.name = "duplicate";
    desc.category = "test";
    desc.allocatorName = "alloc";
    desc.description = "desc";
    desc.factory = nullptr;

    registry.Register(desc);
    registry.Register(desc);

    EXPECT_EQ(registry.Count(), 2u);
}

// Test filter with wildcards
TEST_F(ExperimentRegistryTest, FilterWithWildcards) {
    ExperimentDescriptor desc1;
    desc1.name = "arena/test1";
    desc1.category = "arena";
    desc1.allocatorName = "alloc";
    desc1.description = "Arena test 1";
    desc1.factory = nullptr;

    ExperimentDescriptor desc2;
    desc2.name = "arena/test2";
    desc2.category = "arena";
    desc2.allocatorName = "alloc";
    desc2.description = "Arena test 2";
    desc2.factory = nullptr;

    ExperimentDescriptor desc3;
    desc3.name = "allocator/test1";
    desc3.category = "allocator";
    desc3.allocatorName = "alloc";
    desc3.description = "Allocator test";
    desc3.factory = nullptr;

    registry.Register(desc1);
    registry.Register(desc2);
    registry.Register(desc3);

    const ExperimentDescriptor* results[10];

    // Filter by prefix wildcard
    u32 count = registry.Filter("arena/*", results, 10);
    ASSERT_EQ(count, 2u);
    EXPECT_STREQ(results[0]->name, "arena/test1");
    EXPECT_STREQ(results[1]->name, "arena/test2");

    // Filter by contains wildcard
    count = registry.Filter("*test1", results, 10);
    ASSERT_EQ(count, 2u);

    // Filter no match
    count = registry.Filter("nonexistent*", results, 10);
    EXPECT_EQ(count, 0u);
}
