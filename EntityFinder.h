#ifndef _ENTITY_FINDER_H
#define _ENTITY_FINDER_H

#include <vector>
#include <utility>
#include <string>
#include <iostream>
#include <unordered_map>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/utility.hpp>

#include "WikidataEntity.h"
#include "EntitySearchResult.h"

enum class SearchMode {
  All, Subjects, Properties, Invalid};


BOOST_SERIALIZATION_SPLIT_FREE(std::streampos)
namespace boost {
  namespace serialization {
    template<class Archive>
      void serialize(Archive & ar, std::mbstate_t& s, const unsigned int version) {
        ar & s.__count;
        ar & s.__value.__wch;
      }

    template<class Archive>
      void save(Archive& ar, const std::streampos &s, const unsigned int version) 
      {
        std::streamoff off = s;
        ar & off;
        std::mbstate_t state = s.state();
        ar & state;
      }

    template<class Archive>
      void load(Archive& ar, std::streampos &s, const unsigned int version)
      {
        std::streamoff off;
        ar & off;
        std::mbstate_t state;
        ar & state;
        s += off;
        s.state(state);
      }
  }
}

// ___________________________________________________________
class EntityFinder {

 public:
   using IdxVec = std::vector<std::pair<size_t, size_t>>;
   using AliasIt =
       std::vector<std::pair<std::string, unsigned>>::const_iterator;
   static std::pair<size_t, EntityType>
   getIdxFromWdName(const std::string &wdName);

   // public:
   // Construct from file prepared by Preprocessor
   void InitializeFromTextFile(const std::string &filename);
   void WriteToFile(const std::string &filename);
   static EntityFinder ReadFromFile(const std::string &filename);
   static EntityFinder SetupFromFilename(const std::string &filename);

   EntitySearchResult findEntitiesByPrefix(const std::string &prefix,
                                           SearchMode mode = SearchMode::All);

   std::vector<std::vector<WikidataEntityShort>>
   wdNamesToEntities(const std::vector<std::vector<string>> &wdNames);
   std::vector<WikidataEntityShort>
   wdNamesToEntities(const std::vector<string> &wdNames);
   WikidataEntityShort wdNamesToEntities(const std::string &wdNames) const;

 private:
   // ______________________________________________________________________
   EntitySearchResult convertIdxVecsToSearchResult(const IdxVec &exactIndices,
                                                   const IdxVec &prefixIndices,
                                                   const EntityVectors &v);
   // _______________________________________________________________________
   std::pair<IdxVec, IdxVec> rankResults(AliasIt lower, AliasIt upperExact,
                                         AliasIt upperPrefixes,
                                         const EntityVectors &v);
   const size_t RESULTS_TO_SEND = 40;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version);

   std::string descriptionFilename;

   EntityVectors _entityVecs;
   EntityVectors _propertyVecs;
};



#endif  // _ENTITY_FINDER_H
