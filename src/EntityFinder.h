#ifndef _ENTITY_FINDER_H
#define _ENTITY_FINDER_H

#include <vector>
#include <utility>
#include <string>
#include <iostream>
#include <unordered_map>
#include <gtest/gtest.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/utility.hpp>

#include "WikidataEntityParse.h"
#include "EntityFinderHelpers.h"



// ___________________________________________________________
class EntityFinder {

 public:
   // helper struct used in ranking
   struct IdxVecEntry {
     size_t _numSitelinks;
     size_t _idx;
     IdxVecEntry(size_t numLinks, size_t idx): _numSitelinks(numLinks), _idx(idx) {}
   };
   using IdxVec = std::vector<IdxVecEntry>;
   using AliasIt =
       std::vector<std::pair<std::string, size_t>>::const_iterator;

   // Factory function. if prefix.preprocessed.dat exists, initialize from this file/
   // Else we require prefix.entities and prefix.desc files to initialize and
   // afterwards write prefix.preprocessed.dat for faster startup
   static EntityFinder SetupFromFilePrefix(const std::string &prefix);

   // Find all entities that match a certain prefix. Must explicitly state if we are
   // searching for Subjects (Q...) or Properties (P...)
   std::vector<WikidataEntity> findEntitiesByPrefix(string prefix,
                                                    SearchMode mode = SearchMode::Subjects);
   // Convert from <Q42> (internal format) or <www.wikidata.org/..../Q42>
   // To the corresponding Wikidata Entity.
   // If no information is found, or the input has another format
   // (e.g. "literal"@en) then only the readable name is set.
   std::vector<std::vector<WikidataEntity>>
   wdNamesToEntities(const std::vector<std::vector<string>> &wdNames);

   // overload for not-nested vectors, behaves same as above
   std::vector<WikidataEntity>
   wdNamesToEntities(const std::vector<string> &wdNames);

   // overload for single strings. Is used by the other overloads
   WikidataEntity wdNamesToEntities(const std::string &wdNames) const;

 private:

  // Converts "P31" to <31, EntityType::Property>
  static std::pair<size_t, EntityType>
  getIdxFromWdName(const std::string &wdName);

  // Construct from file prepared by Preprocessor
  void InitializeFromTextFile(const std::string &filename);

  // Serialize this entity finder to a file with the given filename
  void SerializeToFile(const std::string &filename);

  // Read from a binary file that was created from a
  // previous call to SerializeToFile
  static EntityFinder ReadFromSerializedFile(const std::string &filename);

  // Helper function: converts <http://wikidata.org...../Q42> to Q42
  // Strings that do not match this pattern are returned without changes.
  std::string ExtractWikidataIdFromUri(const string& uri) const;

  // ______________________________________________________________________
  std::vector<WikidataEntity> convertIdxVecsToSearchResult(const IdxVec &exactIndices,
                                                              const IdxVec &prefixIndices,
                                                              const EntityVectors &v);
  // Compute the ranking for the given search results and eliminate duplicates
  // lower is the lower bound iterator for the matches,
  // upperExact is the upper bound iterator for the exact matches
  // upperPrefixes is the upper bound iterator for the prefix matches.
  // Argument v is used to correctly get the number of sitelinks.
  std::pair<IdxVec, IdxVec> rankResults(AliasIt lower, AliasIt upperExact,
                                         AliasIt upperPrefixes,
                                         const EntityVectors &v);
  FRIEND_TEST(EntityFinderTest, rankResults);
  // Number of top results that are actually returned from the findEntitiesByPrefix functions
  const size_t RESULTS_TO_SEND = 40;

  // Helper function for Serialization
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version);

  // Information about all the subject(Q...) and property(P...) items
  // that are stored in this class.
  EntityVectors _subjectVecs;
  EntityVectors _propertyVecs;
};



#endif  // _ENTITY_FINDER_H
