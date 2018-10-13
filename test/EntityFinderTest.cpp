// Copyright 2018 Johannes Kalmbach
// Author <johannes.kalmbach@gmail.com>
//

#include <gtest/gtest.h>
#include <string>
#include "../src/EntityFinder.h"

using std::string;

TEST(EntityFinderTest, rankResults) {
   EntityVectors v;
    // create two entities
    v._entities.emplace_back("Q500", "Douglas", "perfume", 60);
    v._entities.emplace_back("Q42", "Douglas Adams", "some writer", 120);

    // one alias for the perfume
    v._aliases.emplace_back("Douglas", 0);
    // two aliases for the writer
    v._aliases.emplace_back("Douglas", 1);
   v._aliases.emplace_back("Douglas Adams", 1);
   // another alias for the perfume
   v._aliases.emplace_back("Douglas Perfume", 0);

   // The aliases are in alphabetical order as required

   // Mocking the search for "douglas", two exact matches and two prefix matches
   auto lower = v._aliases.begin();
   auto upperExact = v._aliases.begin() + 2; // first non-exact match
   auto upperPrefixes = v._aliases.end(); // first not-at-all match

   EntityFinder finder;
   auto resPair = finder.rankResults(lower, upperExact, upperPrefixes, v);
   const auto& exact = resPair.first;
   const auto& prefix = resPair.second;
   ASSERT_EQ(2, exact.size());
   // in the exact matches we want the writer (id 1) first because of the higher
   // sitelink score
   ASSERT_EQ(1, exact[0]._idx);
   ASSERT_EQ(0, exact[1]._idx);
   ASSERT_EQ(2, prefix.size());
   // same goes for the prefix matches.
   // note: when an entity occurs in prefix and exact matches we do not eliminate the duplicate
   ASSERT_EQ(1, prefix[0]._idx);
   ASSERT_EQ(0, prefix[1]._idx);

   // Mocking a search for "doug", 0 exact matches and 4 prefix matches, two of them are duplicates
   // and will be eliminated

   lower = v._aliases.begin();
   upperExact = v._aliases.begin();
   upperPrefixes = v._aliases.end();

    resPair = finder.rankResults(lower, upperExact, upperPrefixes, v);
    const auto& exact2 = resPair.first;
    const auto& prefix2 = resPair.second;
    ASSERT_EQ(0, exact2.size());

    // only two matches because of duplicate elimination
    ASSERT_EQ(2, prefix2.size());
    // first the writer and then the perfume
    ASSERT_EQ(1, prefix2[0]._idx);
    ASSERT_EQ(0, prefix2[1]._idx);
}

// ________________________________________________________________________________
TEST(EntityFinderTest, convertIdxVecs) {
    EntityVectors v;
    // create two entities
    v._entities.emplace_back("Q500", "Douglas", "perfume", 60);
    v._entities.emplace_back("Q42", "Douglas Adams", "some writer", 120);

    // one alias for the perfume
    v._aliases.emplace_back("Douglas", 0);
    // two aliases for the writer
    v._aliases.emplace_back("Douglas", 1);
    v._aliases.emplace_back("Douglas Adams", 1);
    // another alias for the perfume
    v._aliases.emplace_back("Douglas Perfume", 0);


    EntityFinder::IdxVec exactMatches;
    exactMatches.emplace_back(120, 1);
    exactMatches.emplace_back(60, 0);
    EntityFinder::IdxVec prefixMatches;
    prefixMatches.emplace_back(120, 1);
    prefixMatches.emplace_back(60, 0);

    EntityFinder finder;
    auto converted = finder.convertIdxVecsToSearchResult(exactMatches, prefixMatches, v);
    ASSERT_EQ(4, converted.size());

    ASSERT_EQ(std::string("Q42"), converted[2]._wdName);
    ASSERT_EQ(std::string("Douglas Adams"), converted[2]._readableName);

    // test with a reduced send limit
    finder._resultsToSend = 3;
  converted.clear();

  converted = finder.convertIdxVecsToSearchResult(exactMatches, prefixMatches, v);
  ASSERT_EQ(3, converted.size());
  ASSERT_EQ(std::string("Q42"), converted[0]._wdName);
  ASSERT_EQ(std::string("Q42"), converted[0]._wdName);
    ASSERT_EQ(std::string("Douglas Adams"), converted[0]._readableName);
    ASSERT_EQ(std::string("Q500"), converted[1]._wdName);
    ASSERT_EQ(std::string("Douglas"), converted[1]._readableName);
    ASSERT_EQ(std::string("Q42"), converted[2]._wdName);
    ASSERT_EQ(std::string("Douglas Adams"), converted[2]._readableName);

    // test with one of the vectors empty
    exactMatches.clear();
  converted.clear();
  converted = finder.convertIdxVecsToSearchResult(exactMatches, prefixMatches, v);
  ASSERT_EQ(2, converted.size());
  ASSERT_EQ(std::string("Q42"), converted[0]._wdName);
  ASSERT_EQ(std::string("Douglas Adams"), converted[0]._readableName);
  ASSERT_EQ(std::string("Q500"), converted[1]._wdName);
  ASSERT_EQ(std::string("Douglas"), converted[1]._readableName);
}

TEST(EntityFinderTest, initializeFromTextFile) {
  EntityFinder finder;
  finder.InitializeFromTextFile("examples/Q42");
  auto& v = finder._subjectVecs;
  ASSERT_EQ(v._entities.size(), 1);
  auto& ent = v._entities[0];
  ASSERT_EQ(string("<Q42>"),ent._wdName);
  ASSERT_EQ(string("Douglas Adams"), ent._readableName);
  ASSERT_EQ(string("British author and humorist (1952–2001)"), ent._description);
  ASSERT_EQ(109, ent._numSitelinks);
  ASSERT_EQ(EntityType::Subject, ent._type);

  ASSERT_EQ(v._aliases.size(), 4);
  auto& a = v._aliases;
  // aliases must be lowercase and alphabetically sorted now
  ASSERT_EQ(a[0].first, "douglas adams");
  ASSERT_EQ(a[2].first, "douglas noel adams");
  ASSERT_EQ(a[1].first, "douglas n. adams");
  ASSERT_EQ(a[3].first, "douglas noël adams");

  ASSERT_EQ(1, v._entityToIdx.size());
  ASSERT_TRUE(v._entityToIdx.count(42));
  ASSERT_EQ(0, v._entityToIdx[42]);

  // read from file that does not exist
  EntityFinder finder2;
  finder2.InitializeFromTextFile("examples/Notexistent");
  auto & sub = finder2._subjectVecs;
  auto& pred = finder2._propertyVecs;
  ASSERT_TRUE(sub._entities.empty());
  ASSERT_TRUE(pred._entities.empty());
  ASSERT_TRUE(sub._aliases.empty());
  ASSERT_TRUE(pred._aliases.empty());
  ASSERT_TRUE(sub._entityToIdx.empty());
  ASSERT_TRUE(pred._entityToIdx.empty());
}

TEST(EntityFinderTest, extractWikidataIdFromUri) {
  string wd = "<http://www.wikidata.org/entities/Q42>";
  ASSERT_EQ(EntityFinder::ExtractWikidataIdFromUri(wd), string("<Q42>"));
  wd = "<http://www.wikidata.org/prop/path/long/P31>";
  ASSERT_EQ(EntityFinder::ExtractWikidataIdFromUri(wd), string("<P31>"));

  // literal
  wd = "no Entity";
  ASSERT_EQ(EntityFinder::ExtractWikidataIdFromUri(wd), wd);

  // entitiy not from Wikidata
  wd = "<no Wikidata Entity>";
  ASSERT_EQ(EntityFinder::ExtractWikidataIdFromUri(wd), wd);

  // not starting with P.. or Q..
  wd = "<http:://wikidata.org/entities/X52>";
  ASSERT_EQ(EntityFinder::ExtractWikidataIdFromUri(wd), wd);

  // not a numeric Q... id
  wd = "<http:://wikidata.org/entities/Q5a>";
  ASSERT_EQ(EntityFinder::ExtractWikidataIdFromUri(wd), wd);
}

TEST(EntityFinderTest, setupFromFilePrefix) {
  unlink("examples/Q42.preprocessed.dat");
  EntityFinder f = EntityFinder::SetupFromFilePrefix("examples/Q42");
  auto& v = f._subjectVecs;
  ASSERT_EQ(v._entities.size(), 1);
  auto& ent = v._entities[0];
  ASSERT_EQ(string("<Q42>"),ent._wdName);
  ASSERT_EQ(string("Douglas Adams"), ent._readableName);
  ASSERT_EQ(string("British author and humorist (1952–2001)"), ent._description);
  ASSERT_EQ(109, ent._numSitelinks);
  ASSERT_EQ(EntityType::Subject, ent._type);

  ASSERT_EQ(v._aliases.size(), 4);
  auto& a = v._aliases;
  // aliases must be lowercase and alphabetically sorted now
  ASSERT_EQ(a[0].first, "douglas adams");
  ASSERT_EQ(a[2].first, "douglas noel adams");
  ASSERT_EQ(a[1].first, "douglas n. adams");
  ASSERT_EQ(a[3].first, "douglas noël adams");

  ASSERT_EQ(1, v._entityToIdx.size());
  ASSERT_TRUE(v._entityToIdx.count(42));
  ASSERT_EQ(0, v._entityToIdx[42]);


  // now we should have written the preprocessed file
  // test the restart procedure
  EntityFinder f2 = EntityFinder::SetupFromFilePrefix("examples/Q42");
  auto& v2 = f2._subjectVecs;
  ASSERT_EQ(v2._entities.size(), 1);
  auto& ent2 = v2._entities[0];
  ASSERT_EQ(string("<Q42>"),ent2._wdName);
  ASSERT_EQ(string("Douglas Adams"), ent2._readableName);
  ASSERT_EQ(string("British author and humorist (1952–2001)"), ent2._description);
  ASSERT_EQ(109, ent2._numSitelinks);
  ASSERT_EQ(EntityType::Subject, ent2._type);

  ASSERT_EQ(v2._aliases.size(), 4);
  auto& a2 = v2._aliases;
  // aliases must be lowercase and alphabetically sorted now
  ASSERT_EQ(a2[0].first, "douglas adams");
  ASSERT_EQ(a2[2].first, "douglas noel adams");
  ASSERT_EQ(a2[1].first, "douglas n. adams");
  ASSERT_EQ(a2[3].first, "douglas noël adams");

  ASSERT_EQ(1, v2._entityToIdx.size());
  ASSERT_TRUE(v2._entityToIdx.count(42));
  ASSERT_EQ(0, v2._entityToIdx[42]);
}

TEST(EntityFinderTest, findEntitiesByPrefix) {
  EntityFinder f;
  f.InitializeFromTextFile("examples/Q42");
  auto res = f.findEntitiesByPrefix("nomatch", SearchMode::Subjects);
  ASSERT_TRUE(res.empty());

  res = f.findEntitiesByPrefix("DOUG");
  ASSERT_EQ(res.size(), 1);
  ASSERT_EQ(string("<Q42>"), res[0]._wdName);

  res = f.findEntitiesByPrefix("DOUGlas N. Adams");
  ASSERT_EQ(res.size(), 1);
  ASSERT_EQ(string("<Q42>"), res[0]._wdName);


}


