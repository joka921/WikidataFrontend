#ifndef _SERVER_UTILS_H
#define _SERVER_UTILS_H

#include <vector>
#include <string>
#include <utility>


#include "WikidataEntityParse.h"
#include "EntityFinder.h"
#include "EntityFinderHelpers.h"

class QuerySettings {
 public:
   bool asc;
   int orderIdx;
   bool isValid = false;
};
class ServerUtils {
 public:
  static string entitiesToJson(const std::vector<std::vector<WikidataEntity>> &entities);
  // num = 0 means "convert all"
  static string entitiesToJson(const std::vector<WikidataEntity> &entities);
  static std::pair<std::string, SearchMode> parseQuery(const std::string& query);
  static std::string decodeURL(std::string encoded);
  static std::pair<bool, std::string> readFile(std::string filename);
  static std::pair<bool, std::string> detectContentType(std::string filename); 
  static std::vector<std::string> split(const std::string& s, char delim);
};

#endif  // _SERVER_UTILS_H
