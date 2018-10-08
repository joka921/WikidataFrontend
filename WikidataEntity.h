#ifndef _WIKIDATA_ENTITY_H
#define _WIKIDATA_ENTITY_H

#include "./third_party/cereal/include/cereal/archives/json.hpp"
#include "./third_party/cereal/include/cereal/cereal.hpp"
#include "./third_party/json/include/nlohmann/json.hpp"
#include "./third_party/picojson/picojson.h"
#include <iostream>
#include <string>
#include <vector>
<<<<<<< HEAD
#include <iostream>
#include "./cereal/include/cereal/cereal.hpp"
#include "./cereal/include/cereal/archives/json.hpp"
#include "./picojson/picojson.h"

=======
>>>>>>> 95ee882e177cb511267d054f782c56b6e321f305

using std::string;
using json = nlohmann::json;

// _________________________________________________________________________-
enum class EntityType {
  Subject, Property};

inline std::string entityTypeToString(EntityType t) {
  return t == EntityType::Subject ? "Q" : "P";
}

// ________________________________________________________________
enum class OrderType {
  None, NumSitelinks, Numeric, Alphabetical
};

// Converter Function
std::string EntityTypeToString(const EntityType& type); 

// a simple class handling metadata of a wikidata entity
class WikidataEntity {
 public:
  string name;
  string description;
  std::vector<string> aliases;
  unsigned int numSitelinks;
    
  
  // read line that python preprocessor outputs
  // TODO: document format when finished
  WikidataEntity(const string& line);


  // _____________________________________
  static bool IsPropertyName(const std::string& name) {
    return name.substr(0, 2) == std::string("<P");
  }

  // _________________________________________________________________________
  static bool IsSubjectName(const std::string& name) {
    return name.substr(0, 2) == std::string("<Q");
  }
};

// for search results, only index, name and description
class WikidataEntityShort {
 public:
  string wdName;
  string name;
  string description;
  int numSitelinks;
  EntityType type;

  WikidataEntityShort(const string& wd, const string& nameT, const string& desc, unsigned int nSitelinks = 0)
    : wdName(wd), name(nameT), description(desc), numSitelinks(nSitelinks) {
    type = WikidataEntity::IsPropertyName(wdName) ? EntityType::Property : EntityType::Subject;}

  // default constructor needed for resize etc
  WikidataEntityShort() = default;

  static json
  nestedVecToArray(const std::vector<std::vector<WikidataEntityShort>> &vec);

  // _____________________________________________________________________________
  static void sortVec(std::vector<std::vector<WikidataEntityShort>>& vec, size_t orderVarIdx, OrderType type, bool asc);

  string toString() {return wdName + "\t" + name + "\t" + description;}
  static void to_json(json &j, const WikidataEntityShort &ent);

  // TODO: should probably not be here
  static int literalToInt(const std::string &str);

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    ar(CEREAL_NVP(wdName), CEREAL_NVP(name), CEREAL_NVP(description),
       CEREAL_NVP(numSitelinks), CEREAL_NVP(type));
    }

};

#endif  // _WIKIDATA_ENTITY_H
