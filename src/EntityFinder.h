// Copyright 2018 Johannes Kalmbach
// Author <johannes.kalmbach@gmail.com>
//

#ifndef _ENTITY_FINDER_H
#define _ENTITY_FINDER_H

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

#include "EntityFinderHelpers.h"
#include "WikidataEntityParse.h"

// ___________________________________________________________
class EntityFinder {
 public:
  // helper struct used in ranking
  struct IdxVecEntry {
    size_t _numSitelinks;
    size_t _idx;
    IdxVecEntry(size_t numLinks, size_t idx)
        : _numSitelinks(numLinks), _idx(idx) {}
  };

  using IdxVec = std::vector<IdxVecEntry>;
  // const iterator to the alias vectors
  using AliasIt = std::vector<std::pair<std::string, size_t>>::const_iterator;

  // Factory function. if prefix.preprocessed.dat exists, initialize from this
  // file/ Else we require prefix.entities and prefix.desc files to initialize
  // and afterwards write prefix.preprocessed.dat for faster startup
  static EntityFinder SetupFromFilePrefix(const std::string& prefix);
  FRIEND_TEST(EntityFinderTest, setupFromFilePrefix);

  // Find all entities that match a certain prefix. Must explicitly state if we
  // are searching for Subjects (Q...) or Properties (P...)
  std::vector<WikidataEntity> findEntitiesByPrefix(
      string prefix, SearchMode mode = SearchMode::Subjects) const;
  FRIEND_TEST(EntityFinderTest, findEntitiesByPrefix);

  // Convert from <Q42> (internal format) or <www.wikidata.org/..../Q42>
  // To the corresponding Wikidata Entity.
  // If no information is found, or the input has another format
  // (e.g. "literal"@en) then only the readable name is set.
  std::vector<std::vector<WikidataEntity>> wdNamesToEntities(
      const std::vector<std::vector<string>>& wdNames) const;

  // overload for not-nested vectors, behaves same as above
  std::vector<WikidataEntity> wdNamesToEntities(
      const std::vector<string>& wdNames) const;

  // overload for single strings. Is used by the other overloads
  WikidataEntity wdNamesToEntities(const std::string& wdNames) const;

 private:
  // Converts "P31" to <31, EntityType::Property>
  static std::pair<size_t, EntityType> getIdxFromWdName(
      const std::string& wdName);

  // Construct from file prepared by Preprocessor
  void InitializeFromTextFile(const std::string& filePrefix);
  FRIEND_TEST(EntityFinderTest, initializeFromTextFile);

  // Serialize this entity finder to a file with the given filename
  // If the file can not be opened, nothing is done and a warning is emitted
  // This method relies only on boost::serialization and thus is not tested
  void SerializeToFile(const std::string& filename) const;

  // Read from a binary file that was created from a
  // previous call to SerializeToFile
  // This method relies only on boost::serialization and thus is not tested
  static EntityFinder ReadFromSerializedFile(const std::string& filename);

  // Helper function: converts <http://wikidata.org...../Q42> to Q42
  // Strings that do not match this pattern are returned without changes.
  static std::string ExtractWikidataIdFromUri(const string& uri);
  FRIEND_TEST(EntityFinderTest, extractWikidataIdFromUri);

  // ______________________________________________________________________
  std::vector<WikidataEntity> convertIdxVecsToSearchResult(
      const IdxVec& exactIndices, const IdxVec& prefixIndices,
      const EntityVectors& v) const;
  FRIEND_TEST(EntityFinderTest, convertIdxVecs);
  // Compute the ranking for the given search results and eliminate duplicates
  // lower is the lower bound iterator for the matches,
  // upperExact is the upper bound iterator for the exact matches
  // upperPrefixes is the upper bound iterator for the prefix matches.
  // Argument v is used to correctly get the number of sitelinks.
  std::pair<IdxVec, IdxVec> rankResults(AliasIt lower, AliasIt upperExact,
                                        AliasIt upperPrefixes,
                                        const EntityVectors& v) const;
  FRIEND_TEST(EntityFinderTest, rankResults);
  // Number of top results that are actually returned from the
  // findEntitiesByPrefix functions
  size_t _resultsToSend = 40;

  // Helper function for Serialization
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  // Information about all the subject(Q...) and property(P...) items
  // that are stored in this class.
  EntityVectors _subjectVecs;
  EntityVectors _propertyVecs;
};

#endif  // _ENTITY_FINDER_H
