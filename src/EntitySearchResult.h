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

class EntityVectors {
public:
  // all the aliases and the index they refer to
  std::vector<std::pair<std::string, unsigned>> aliasVec;
  std::vector<std::string> nameVec; // readable names
  std::vector<std::string> descVec; // descriptions
  // in wikidata dumps, the entries are not ordered, so we have to keep track
  // of their indices
  //  if EntityToIdxVec[x] = y then entity "Qx" can be found at idx y, same
  //  for properties
  std::vector<size_t> EntityToIdxVec;
  std::vector<unsigned> numSitelinkVec;
  std::vector<std::string> wdNameVec; // Name in Wikidata, e.gl "Q23"

  // _____________________________________________________________
  void shrink_to_fit() {
    EntityToIdxVec.shrink_to_fit();
    aliasVec.shrink_to_fit();
    descVec.shrink_to_fit();
    nameVec.shrink_to_fit();
    numSitelinkVec.shrink_to_fit();
    wdNameVec.shrink_to_fit();
  }

private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) {
    ar &wdNameVec;
    ar &EntityToIdxVec;
    ar &descVec;
    ar &nameVec;
    ar &aliasVec;
    ar &numSitelinkVec;
  }
};

#endif  // _ENTITY_SEARCH_RESULT_H
