#include <array>
#include "gtest/gtest.h"
#include "movie_writer.hpp"

class SnapTest : public ::testing::Test {
 protected:
  void SetUp() override {
  }

  // void TearDown() override {}

};

TEST_F(SnapTest, SnapWorks)
{
	std::array<Rational, 14> standardRates{
        Rational{24000, 1001},
        Rational{   24,    1},
        Rational{   25,    1},
        Rational{30000, 1001},
        Rational{   30,    1},
        Rational{48000, 1001},   // unofficial, but may be found in the wild
        Rational{   50,    1},
        Rational{60000, 1001},
        Rational{   60,    1},
        Rational{   15,    1},
        Rational{    5,    1},
        Rational{   10,    1},
        Rational{   12,    1},
        Rational{   15,    1}
    };

    // standard rates should pass unhindered
    for (auto r:standardRates)
    {
		EXPECT_EQ(r, SimplifyAndSnapToMpegFrameRate(r));
    }
    // test large deviations passthrough
    {
    	Rational r{1000, 1};
		EXPECT_EQ(r, SimplifyAndSnapToMpegFrameRate(r));
	}
    {
    	Rational r{2, 1};
		EXPECT_EQ(r, SimplifyAndSnapToMpegFrameRate(r));
	}

    // test small deviations get snapped
    {
    	Rational standard{30000, 1001};
    	Rational big{30000 * 65536, 1001 * 65536};
    	Rational deviated1{30000 * 65536+1, 1001 * 65536};
    	Rational deviated2{30000 * 65536, 1001 * 65536+1};
    	Rational deviated3{30000 * 65536-1, 1001 * 65536};
    	Rational deviated4{30000 * 65536, 1001 * 65536-1};

		EXPECT_EQ(standard, SimplifyAndSnapToMpegFrameRate(big));
		EXPECT_EQ(standard, SimplifyAndSnapToMpegFrameRate(deviated1));
		EXPECT_EQ(standard, SimplifyAndSnapToMpegFrameRate(deviated2));
		EXPECT_EQ(standard, SimplifyAndSnapToMpegFrameRate(deviated3));
		EXPECT_EQ(standard, SimplifyAndSnapToMpegFrameRate(deviated4));
	}
}
