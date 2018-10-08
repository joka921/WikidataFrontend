#ifndef _WIKIDATA_ENTITY_H
#define _WIKIDATA_ENTITY_H

#include <iostream>
#include <string>
#include <vector>
#include <iostream>

using std::string;
using json = nlohmann::json;

// Wikidata has items/subjects (Q42) and Properties(P31)
enum class EntityType {
  Subject, Property};

// Simple Conversion function
inline std::string entityTypeToString(EntityType t) {
  return t == EntityType::Subject ? "Q" : "P";
}

// Different Aspects after which we can sort entities
enum class OrderType {
  None, NumSitelinks, Numeric, Alphabetical
};

// Converter Function
// TODO: Rename/Refactor this
std::string EntityTypeToString(const EntityType& type);

// Class for the metadata of a wikidata entity
class WikidataEntity {
 public:
  string name;
  string description;
  std::vector<string> aliases;
  unsigned int numSitelinks;

  // Read from line of the entity/alias File
  // (see README). Is not able to parse the description
  // which has to be set manually
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

// A smaller class for the meta data of a Wikidata entitye
// Used for results of searches and queries
// Does not need the aliases
class WikidataEntityShort {
 public:
  string wdName;
  string name;
  string description;
  int numSitelinks;
  EntityType type;

  // Constructor
  WikidataEntityShort(const string& wd, const string& nameT, const string& desc, unsigned int nSitelinks = 0)
    : wdName(wd), name(nameT), description(desc), numSitelinks(nSitelinks) {
    type = WikidataEntity::IsPropertyName(wdName) ? EntityType::Property : EntityType::Subject;}

  // default constructor needed for resize etc
  WikidataEntityShort() = default;

  // TODO: not needed with new JSON library
  static json
  nestedVecToArray(const std::vector<std::vector<WikidataEntityShort>> &vec);

  // _____________________________________________________________________________
  // TODO: we are probably not doing the sorting here but in QLever
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
