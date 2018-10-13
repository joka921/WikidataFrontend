// Copyright 2018 Johannes Kalmbach
// Author <johannes.kalmbach@gmail.com>
//
#ifndef _ENTITY_SEARCH_RESULT_H
#define _ENTITY_SEARCH_RESULT_H

#include "WikidataEntityParse.h"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include <unordered_map>

enum class SearchMode {
  Subjects, Properties, Invalid};

// Helper class for the internal data structure of the EntityFinder.
// This information is needed twice, once for properties and one for subjects/ objects
class EntityVectors {
public:
  // All the entities of which we store information.
  std::vector<WikidataEntity> _entities;
  // All the aliases and the index in the _entities member they they refer to
  std::vector<std::pair<std::string, size_t>> _aliases;

  //  if _entityToIdx[42] = 15 then entity "Q42" can be found at idx 15 in
  //  the _entities member, same
  std::unordered_map<size_t, size_t> _entityToIdx;

  // _____________________________________________________________
  void shrink_to_fit() {
    _aliases.shrink_to_fit();
    _entities.shrink_to_fit();
  }

private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) {
    (void) version;
    ar &_entities;
    ar &_entityToIdx;
    ar &_aliases;
  }
};

#endif  // _ENTITY_SEARCH_RESULT_H
