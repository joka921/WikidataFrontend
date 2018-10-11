// Copyright 2016 Johannes Kalmbach
// <johannes.kalmbach@gmail.com
//
// Test for helper functions of Search server in utils.h


#include <string>

#include <gtest/gtest.h>
#include "../src/ServerUtils.h"

using std::string;

// ________________________________________________________
TEST(SearchServerUtils, detectContentType) {
  std::string filename = "test.txt";
  auto res = ServerUtils::detectContentType(filename);
  ASSERT_TRUE(res.first);
  ASSERT_STREQ(res.second.c_str(), "text/plain");

  filename = "text.js";
  res = ServerUtils::detectContentType(filename);
  ASSERT_TRUE(res.first);
  ASSERT_STREQ(res.second.c_str(), "application/javascript");

  filename = "invalid.exe";
  res = ServerUtils::detectContentType(filename);
  ASSERT_FALSE(res.first);

  filename = "nodot";
  res = ServerUtils::detectContentType(filename);
  ASSERT_FALSE(res.first);
}

// ___________________________________________________________
TEST(SearchServerUtils, readFile) {
  std::string filename = "noexist.txt";
  auto res = ServerUtils::readFile(filename);
  ASSERT_FALSE(res.first);

  filename = "examples/Q42.desc";
  res = ServerUtils::readFile(filename);
  ASSERT_TRUE(res.first);
  std::string comp = "British author and humorist (1952\xE2\x80\x93" "2001)\n";


  ASSERT_STREQ(res.second.c_str(), comp.c_str());
}

TEST(SearchServerUtils, decodeURL) {
  ASSERT_STREQ(ServerUtils::decodeURL("z%C3%BCrich").c_str(), "zürich");
  ASSERT_STREQ(ServerUtils::decodeURL("L%C3%B8kken").c_str(), "Løkken");
  ASSERT_STREQ(ServerUtils::decodeURL("a+o").c_str(), "a o");
  ASSERT_STREQ(ServerUtils::decodeURL("%C3%A1+%C3%A9").c_str(), "á é");
}

TEST(SearchServerUtils, parsePrefixSearchQuery) {
  auto res = ServerUtils::parsePrefixSearchQuery("?t=obj?q=pref");
  ASSERT_EQ(res.second, SearchMode::Subjects);
  ASSERT_EQ(string("pref"), res.first);

  res = ServerUtils::parsePrefixSearchQuery("?t=prd?q=pref");
  ASSERT_EQ(res.second, SearchMode::Properties);
  ASSERT_EQ(string("pref"), res.first);

  res = ServerUtils::parsePrefixSearchQuery("?t=all?q=pref");
  ASSERT_EQ(res.second, SearchMode::Subjects);
  ASSERT_EQ(string("pref"), res.first);

  res = ServerUtils::parsePrefixSearchQuery("?t=allIllformated=pref");
  ASSERT_EQ(res.second, SearchMode::Invalid);

  res = ServerUtils::parsePrefixSearchQuery("?t=non?q=pref");
  ASSERT_EQ(res.second, SearchMode::Invalid);
}
