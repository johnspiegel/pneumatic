#include <dump.h>
#include <unity.h>

using dump::CToF;
using dump::MillisHumanReadable;
using dump::SecondsHumanReadable;

void Test_SecondsHumanReadable() {
  TEST_ASSERT_EQUAL_STRING("0s", SecondsHumanReadable(0).c_str());
  TEST_ASSERT_EQUAL_STRING("0s", SecondsHumanReadable(499).c_str());
  TEST_ASSERT_EQUAL_STRING("1s", SecondsHumanReadable(500).c_str());
  TEST_ASSERT_EQUAL_STRING("1s", SecondsHumanReadable(1000).c_str());
  TEST_ASSERT_EQUAL_STRING("1s", SecondsHumanReadable(1499).c_str());
  TEST_ASSERT_EQUAL_STRING("2s", SecondsHumanReadable(1500).c_str());
  TEST_ASSERT_EQUAL_STRING("59s", SecondsHumanReadable(59 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING("1m00s", SecondsHumanReadable(60 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING("10m00s",
                           SecondsHumanReadable(10 * 60 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING("10m00s",
                           SecondsHumanReadable(10 * 60 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING("1h00m00s",
                           SecondsHumanReadable(1 * 60 * 60 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING("10h00m00s",
                           SecondsHumanReadable(10 * 60 * 60 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING("1d00h00m00s",
                           SecondsHumanReadable(24 * 60 * 60 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING(
      "10d00h00m00s", SecondsHumanReadable(10 * 24 * 60 * 60 * 1000).c_str());

  TEST_ASSERT_EQUAL_STRING(
      "32d12h34m56s",
      SecondsHumanReadable(32ul * 24 * 60 * 60 * 1000 + 12 * 60 * 60 * 1000 +
                           34 * 60 * 1000 + 56 * 1000)
          .c_str());
}

void Test_MillisHumanReadable() {
  TEST_ASSERT_EQUAL_STRING("0.000s", MillisHumanReadable(0).c_str());
  TEST_ASSERT_EQUAL_STRING("0.001s", MillisHumanReadable(1).c_str());
  TEST_ASSERT_EQUAL_STRING("0.020s", MillisHumanReadable(20).c_str());
  TEST_ASSERT_EQUAL_STRING("0.300s", MillisHumanReadable(300).c_str());
  TEST_ASSERT_EQUAL_STRING("0.499s", MillisHumanReadable(499).c_str());
  TEST_ASSERT_EQUAL_STRING("0.500s", MillisHumanReadable(500).c_str());
  TEST_ASSERT_EQUAL_STRING("1.000s", MillisHumanReadable(1000).c_str());
  TEST_ASSERT_EQUAL_STRING("1.499s", MillisHumanReadable(1499).c_str());
  TEST_ASSERT_EQUAL_STRING("1.500s", MillisHumanReadable(1500).c_str());
  TEST_ASSERT_EQUAL_STRING("59.000s", MillisHumanReadable(59 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING("1m00.000s", MillisHumanReadable(60 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING("10m00.000s",
                           MillisHumanReadable(10 * 60 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING("10m00.000s",
                           MillisHumanReadable(10 * 60 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING("1h00m00.000s",
                           MillisHumanReadable(1 * 60 * 60 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING("10h00m00.000s",
                           MillisHumanReadable(10 * 60 * 60 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING("1d00h00m00.000s",
                           MillisHumanReadable(24 * 60 * 60 * 1000).c_str());
  TEST_ASSERT_EQUAL_STRING(
      "10d00h00m00.000s",
      MillisHumanReadable(10 * 24 * 60 * 60 * 1000).c_str());

  TEST_ASSERT_EQUAL_STRING(
      "32d12h34m56.987s",
      MillisHumanReadable(32ul * 24 * 60 * 60 * 1000 + 12 * 60 * 60 * 1000 +
                          34 * 60 * 1000 + 56 * 1000 + 987)
          .c_str());
}

void Test_CToF() {
  TEST_ASSERT_EQUAL_FLOAT(32.0, CToF(0.0));
  TEST_ASSERT_EQUAL_FLOAT(212.0, CToF(100.0));
  TEST_ASSERT_EQUAL_FLOAT(41.0, CToF(5.0));
  TEST_ASSERT_EQUAL_FLOAT(50.0, CToF(10.0));
  TEST_ASSERT_EQUAL_FLOAT(59.0, CToF(15.0));
  TEST_ASSERT_EQUAL_FLOAT(68.0, CToF(20.0));

  TEST_ASSERT_EQUAL_FLOAT(392.0, CToF(200.0));

  TEST_ASSERT_EQUAL_FLOAT(23.0, CToF(-5.0));
  TEST_ASSERT_EQUAL_FLOAT(-4.0, CToF(-20.0));

  TEST_ASSERT_FLOAT_WITHIN(0.0001, 0.0, CToF(-17.7778));
  TEST_ASSERT_FLOAT_WITHIN(0.0001, 74.22206, CToF(23.4567));
  TEST_ASSERT_FLOAT_WITHIN(0.0001, -190.22206, CToF(-123.4567));
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(Test_SecondsHumanReadable);
  RUN_TEST(Test_MillisHumanReadable);
  RUN_TEST(Test_CToF);
  UNITY_END();
}

void loop() {}
