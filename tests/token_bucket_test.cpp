
#include "token_bucket.h"
#include <gtest/gtest.h>

TEST(TokenBucketTest, BasicConsume){
    TokenBucket bucket(10,5);
    int rem; long long reset;
    ASSERT_TRUE(bucket.consume(3,rem,reset));
    EXPECT_EQ(rem,7);
}

TEST(TokenBucketTest, ExhaustAndDeny){
    TokenBucket bucket(5,1);
    int rem; long long reset;
    ASSERT_TRUE(bucket.consume(5,rem,reset));
    ASSERT_FALSE(bucket.consume(1,rem,reset));
    EXPECT_EQ(rem,0);
}
