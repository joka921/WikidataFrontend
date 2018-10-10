//
// Created by johannes on 10.10.18.
//

#include <gtest/gtest.h>
#include "../src/EntityFinder.h"

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
    ASSERT_EQ(std::string("Q42"), converted[0]._wdName);
    ASSERT_EQ(std::string("Douglas Adams"), converted[0]._readableName);
    ASSERT_EQ(std::string("Q500"), converted[1]._wdName);
    ASSERT_EQ(std::string("Douglas"), converted[1]._readableName);
    ASSERT_EQ(std::string("Q42"), converted[2]._wdName);
    ASSERT_EQ(std::string("Douglas Adams"), converted[2]._readableName);

    // check limit
    finder._resultsToSend = 3;
    converted = finder.convertIdxVecsToSearchResult(exactMatches, prefixMatches, v);
    ASSERT_EQ(std::string("Q42"), converted[0]._wdName);
    ASSERT_EQ(3, converted.size());
    ASSERT_EQ(std::string("Q42"), converted[0]._wdName);
    ASSERT_EQ(std::string("Douglas Adams"), converted[0]._readableName);
    ASSERT_EQ(std::string("Q500"), converted[1]._wdName);
    ASSERT_EQ(std::string("Douglas"), converted[1]._readableName);
    ASSERT_EQ(std::string("Q42"), converted[2]._wdName);
    ASSERT_EQ(std::string("Douglas Adams"), converted[2]._readableName);


}