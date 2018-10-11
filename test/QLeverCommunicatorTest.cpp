//
// Created by johannes on 11.10.18.
//

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "../src/QLeverCommunicator.h"
#include "../src/EntityFinder.h"

using json = nlohmann::json;

TEST(QLeverCommunicatorTest, replaceEntitiesInQLeverResult) {
  QLeverCommunicator q("dummyServer", 9999);
  EntityFinder finder = EntityFinder::SetupFromFilePrefix("examples/Q42");

  // that this works sufficiently has been tested elsewhere

  json invalidJson;
  invalidJson["status"] = "ERROR";
  invalidJson["otherValue"] = "garbage";
  auto res = q.replaceEntitiesInQLeverResult(invalidJson.dump(), finder);
  ASSERT_EQ(json::parse(res), invalidJson);

  json missingStatusKey;
  res = q.replaceEntitiesInQLeverResult(missingStatusKey.dump(), finder);
  json resJson = json::parse(res);
  string status = resJson["status"];
  ASSERT_EQ(status, string("ERROR"));

  json illFormattedResult;
  illFormattedResult["status"] = "OK";
  illFormattedResult["res"] = "directly putting a string here";

  res = q.replaceEntitiesInQLeverResult(illFormattedResult.dump(), finder);

  resJson = json::parse(res);
  status = resJson["status"];
  ASSERT_EQ(status, string("OK"));
  ASSERT_EQ(resJson["res"].size(), 1);
  ASSERT_EQ(resJson["res"][0].size(), 1);
  ASSERT_EQ(resJson["res"][0][0]["name"], "directly putting a string here");


  // now for an actually correct search
  json correctResult;
  correctResult["status"] = "OK";
  std::vector<std::vector<string>> results;
  results.emplace_back();
  results[0].push_back("some literal");
  results[0].push_back("<http://www.wikidata.org/entities/Q42>");
  correctResult["res"] = results;

  res = q.replaceEntitiesInQLeverResult(correctResult.dump(), finder);

  resJson = json::parse(res);
  status = resJson["status"];
  ASSERT_EQ(status, string("OK"));
  ASSERT_EQ(resJson["res"].size(), 1);
  ASSERT_EQ(resJson["res"][0].size(), 2);
  string firstName = resJson["res"][0][0]["name"];
  ASSERT_EQ(firstName, string("some literal"));
  string secondName = resJson["res"][0][1]["name"];
  ASSERT_EQ(secondName, string("Douglas Adams"));
}