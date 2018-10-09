#ifndef _SERVER_UTILS_H
#define _SERVER_UTILS_H

#include <vector>
#include <string>
#include <utility>


#include "WikidataEntityParse.h"
#include "EntityFinder.h"
#include "EntitySearchResult.h"

class QuerySettings {
 public:
   OrderType type;
   bool asc;
   int orderIdx;
   bool isValid = false;
};
class ServerUtils {
 public:
  static std::string entitiesToJson(const std::vector<std::vector<WikidataEntityShort>>& entities, size_t num);
  static std::string entitiesToJson(const EntitySearchResult& entities, size_t num);
  static std::string escapeJson(const std::string& wordNarrow);
  static std::pair<std::string, SearchMode> parseQuery(const std::string& query);
  static std::string decodeURL(std::string encoded);
  static std::pair<bool, std::string> readFile(std::string filename);
  static std::pair<bool, std::string> detectContentType(std::string filename); 
  static std::vector<std::string> split(const std::string& s, char delim);
  static QuerySettings parseSetting(const std::string& settingStr);
};

#endif  // _SERVER_UTILS_H
