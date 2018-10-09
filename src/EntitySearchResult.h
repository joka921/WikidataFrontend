#ifndef _ENTITY_SEARCH_RESULT_H
#define _ENTITY_SEARCH_RESULT_H

#include "WikidataEntity.h"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

class EntitySearchResult{
 public:
  std::vector<WikidataEntityShort> entities;
  size_t totalResults;
  bool sorted;
};

enum class SearchMode {
  Subjects, Properties, Invalid};

// Helper class for the internal data structure of the EntityFinder
// This information is needed twice, once for properties and one for subjects/ objects
class EntityVectors {
public:
  // All the entities of which we store information here
  std::vector<WikidataEntityShort> _entities;
  // all the aliases and the index in the _entities member they they refer to
  std::vector<std::pair<std::string, unsigned>> _aliases;

  //  if _entityToIdx[x] = y then entity "Qx" can be found at idx y, same
  //  for properties. Uses size_t(-1) as an empty value.
  // TODO: actually use hashMap for this.
  std::vector<size_t> _entityToIdx;

  // _____________________________________________________________
  void shrink_to_fit() {
    _entityToIdx.shrink_to_fit();
    _aliases.shrink_to_fit();
    _entities.shrink_to_fit();
    /*descVec.shrink_to_fit();
    nameVec.shrink_to_fit();
    numSitelinkVec.shrink_to_fit();
    wdNameVec.shrink_to_fit();
     */
  }

private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) {
    //ar &wdNameVec;
    ar &_entities;
    ar &_entityToIdx;

   // ar &descVec;
    //ar &nameVec;
    ar &_aliases;
   // ar &numSitelinkVec;
  }
};

#endif  // _ENTITY_SEARCH_RESULT_H
