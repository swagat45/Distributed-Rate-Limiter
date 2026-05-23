#include "src/token_bucket.h"
#include <gtest/gtest.h>

namespace {

ClientConfig defaultConfig() {
    return ClientConfig{10, 5, 60};
}

}  // namespace

TEST(TokenBucketMathTest, ValidateConfigAcceptsPositiveValues) {
    EXPECT_NO_THROW(TokenBucketMath::validateConfig(defaultConfig()));
}

TEST(TokenBucketMathTest, ValidateConfigRejectsNonPositiveCapacity) {
    EXPECT_THROW(TokenBucketMath::validateConfig(ClientConfig{0, 5, 60}),
                 std::invalid_argument);
}

TEST(TokenBucketMathTest, ValidateConfigRejectsNonPositiveRefill) {
    EXPECT_THROW(TokenBucketMath::validateConfig(ClientConfig{10, 0, 60}),
                 std::invalid_argument);
}

TEST(TokenBucketMathTest, BasicConsumeSucceedsAndReturnsRemainingTokens) {
    TokenBucketState state{10.0, 1000};
    BucketStat stat{};

    const bool allowed = TokenBucketMath::consume(state, defaultConfig(), 3, 1000, stat);

    EXPECT_TRUE(allowed);
    EXPECT_EQ(7, stat.remaining);
    EXPECT_EQ(600, stat.reset_after_ms);
}

TEST(TokenBucketMathTest, RejectsRequestWhenBucketIsEmpty) {
    TokenBucketState state{0.0, 1000};
    BucketStat stat{};

    const bool allowed = TokenBucketMath::consume(state, defaultConfig(), 1, 1000, stat);

    EXPECT_FALSE(allowed);
    EXPECT_EQ(0, stat.remaining);
    EXPECT_EQ(200, stat.reset_after_ms);
}

TEST(TokenBucketMathTest, RefillsTokensBasedOnElapsedTime) {
    TokenBucketState state{2.0, 1000};
    BucketStat stat{};

    const bool allowed = TokenBucketMath::consume(state, defaultConfig(), 1, 2000, stat);

    EXPECT_TRUE(allowed);
    EXPECT_EQ(6, stat.remaining);
    EXPECT_EQ(800, stat.reset_after_ms);
}

TEST(TokenBucketMathTest, RefillDoesNotExceedCapacity) {
    TokenBucketState state{9.5, 1000};
    BucketStat stat{};

    const bool allowed = TokenBucketMath::consume(state, defaultConfig(), 1, 5000, stat);

    EXPECT_TRUE(allowed);
    EXPECT_EQ(9, stat.remaining);
    EXPECT_EQ(200, stat.reset_after_ms);
}

TEST(TokenBucketMathTest, OverCapacityRequestIsRejected) {
    TokenBucketState state{10.0, 1000};
    BucketStat stat{};

    const bool allowed = TokenBucketMath::consume(state, defaultConfig(), 11, 1000, stat);

    EXPECT_FALSE(allowed);
    EXPECT_EQ(10, stat.remaining);
    EXPECT_EQ(200, stat.reset_after_ms);
}

TEST(TokenBucketMathTest, ZeroTokenConsumeIsRejected) {
    TokenBucketState state{10.0, 1000};
    BucketStat stat{};

    EXPECT_THROW(TokenBucketMath::consume(state, defaultConfig(), 0, 1000, stat),
                 std::invalid_argument);
}

TEST(TokenBucketMathTest, NegativeTokenConsumeIsRejected) {
    TokenBucketState state{10.0, 1000};
    BucketStat stat{};

    EXPECT_THROW(TokenBucketMath::consume(state, defaultConfig(), -1, 1000, stat),
                 std::invalid_argument);
}

TEST(TokenBucketMathTest, QuotaUpdateAddsTokensAndClampsToCapacity) {
    TokenBucketState state{8.0, 1000};
    BucketStat stat{};
    QuotaUpdate update{QuotaUpdate::Mode::kAddTokens, 5, false};

    TokenBucketMath::applyQuotaUpdate(state, defaultConfig(), update, 1000, stat);

    EXPECT_EQ(10, stat.remaining);
    EXPECT_EQ(0, stat.reset_after_ms);
}

TEST(TokenBucketMathTest, QuotaUpdateSetTokensClampsToZero) {
    TokenBucketState state{8.0, 1000};
    BucketStat stat{};
    QuotaUpdate update{QuotaUpdate::Mode::kSetTokens, -4, false};

    TokenBucketMath::applyQuotaUpdate(state, defaultConfig(), update, 1000, stat);

    EXPECT_EQ(0, stat.remaining);
    EXPECT_EQ(2000, stat.reset_after_ms);
}

TEST(TokenBucketMathTest, ReconcileConfigCanResetBucketToFull) {
    TokenBucketState state{1.0, 1000};
    BucketStat stat{};

    state = TokenBucketMath::reconcileConfig(state, defaultConfig(), true, 1500, stat);

    EXPECT_EQ(10, stat.remaining);
    EXPECT_EQ(0, stat.reset_after_ms);
    EXPECT_EQ(1500, state.last_refill_ms);
}

TEST(TokenBucketMathTest, ReconcileConfigClampsExistingTokensToNewCapacity) {
    TokenBucketState state{12.0, 1000};
    BucketStat stat{};
    ClientConfig smaller{6, 3, 60};

    state = TokenBucketMath::reconcileConfig(state, smaller, false, 1000, stat);

    EXPECT_EQ(6, stat.remaining);
    EXPECT_EQ(0, stat.reset_after_ms);
}
