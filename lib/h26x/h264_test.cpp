#include <gtest/gtest.h>

#include "h264.hpp"

#include <vector>

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

TEST(IterateAnnexB, IterateAnnexB) {
    const unsigned char data[] = {0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x01, 0x04};

    size_t counter = 0;
    std::vector<std::vector<uint8_t>> expected = {
        std::vector<uint8_t>({0x01, 0x02, 0x03}),
        std::vector<uint8_t>({0x04}),
    };

    EXPECT_TRUE(h264::IterateAnnexB(data, sizeof(data), [&](const void* data, size_t len) {
        ASSERT_LT(counter, expected.size());
        ASSERT_EQ(expected[counter].size(), len);
        ASSERT_EQ(0, memcmp(&expected[counter][0], data, len));
        ++counter;
    }));

    EXPECT_EQ(expected.size(), counter);
}
