#include <gtest/gtest.h>

#include "h264.hpp"

TEST(ue, decode) {
    h264::ue n;

    {
        h264::bitstream bs{"\x00", 1};
        EXPECT_TRUE(n.decode(&bs));
    }

    {
        h264::bitstream bs{"\x80", 1};
        EXPECT_FALSE(n.decode(&bs));
        EXPECT_EQ(0, n);
    }

    {
        h264::bitstream bs{"\x40", 1};
        EXPECT_FALSE(n.decode(&bs));
        EXPECT_EQ(1, n);
    }

    {
        h264::bitstream bs{"\x60", 1};
        EXPECT_FALSE(n.decode(&bs));
        EXPECT_EQ(2, n);
    }

    {
        h264::bitstream bs{"\x20", 1};
        EXPECT_FALSE(n.decode(&bs));
        EXPECT_EQ(3, n);
    }

    {
        h264::bitstream bs{"\x28", 1};
        EXPECT_FALSE(n.decode(&bs));
        EXPECT_EQ(4, n);
    }
}

TEST(se, decode) {
    h264::se n;

    {
        h264::bitstream bs{"\x00", 1};
        EXPECT_TRUE(n.decode(&bs));
    }

    {
        h264::bitstream bs{"\x80", 1};
        EXPECT_FALSE(n.decode(&bs));
        EXPECT_EQ(0, n);
    }

    {
        h264::bitstream bs{"\x40", 1};
        EXPECT_FALSE(n.decode(&bs));
        EXPECT_EQ(1, n);
    }

    {
        h264::bitstream bs{"\x60", 1};
        EXPECT_FALSE(n.decode(&bs));
        EXPECT_EQ(-1, n);
    }

    {
        h264::bitstream bs{"\x20", 1};
        EXPECT_FALSE(n.decode(&bs));
        EXPECT_EQ(2, n);
    }

    {
        h264::bitstream bs{"\x28", 1};
        EXPECT_FALSE(n.decode(&bs));
        EXPECT_EQ(-2, n);
    }
}
