#ifndef _ENTITY_SEARCH_RESULT_H
#define _ENTITY_SEARCH_RESULT_H

#include "WikidataEntity.h"

class EntitySearchResult{
 public:
  std::vector<WikidataEntityShort> entities;
  size_t totalResults;
  bool sorted;
};

#endif  // _ENTITY_SEARCH_RESULT_H
