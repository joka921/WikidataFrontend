#ifndef _WIKIDATA_ENTITY_H
#define _WIKIDATA_ENTITY_H

#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using std::string;
using json = nlohmann::json;

// Wikidata has items/subjects (Q42) and Properties(P31)
enum class EntityType {
  Subject, Property};

// Convert Subject to "Q" and Property to "P"
inline std::string entityTypeToCharacter(EntityType t) {
  return t == EntityType::Subject ? "Q" : "P";
}
// Convert Entity Type to a single digit. Used in the Javascript Frontend
inline std::string entityTypeToNumeric(const EntityType &t){
  return t == EntityType::Subject ? "0" : "1";
}

// Different Aspects after which we can sort entities
enum class OrderType {
  None, NumSitelinks, Numeric, Alphabetical
};

// Class for the metadata of a wikidata entity
// only used during initial parsing
class WikidataEntityParse {
 public:
  // Wikidata name like "<Q42>" or "<P31>"
  string _wdName;

  // All the english names and synonyms of the entity.
  // The first alias is the readable name of this entity
  std::vector<string> _aliases;
  unsigned int _numSiteLinks;
  // Read from line of the entity/alias File
  // (see README).
  WikidataEntityParse(const string& line);

  // _____________________________________
  static bool isPropertyName(const std::string &name) {
    return name.substr(0, 2) == std::string("<P");
  }

  // _________________________________________________________________________
  static bool isSubjectName(const std::string &name) {
    return name.substr(0, 2) == std::string("<Q");
  }
};

// A smaller class for the meta data of a Wikidata entitye
// Used for results of searches and queries
// Does not need the aliases
class WikidataEntityShort {
 public:
  string _wdName;
  string _readableName;
  string _description;
  int _numSitelinks;
  EntityType _type;

  // Constructor
  WikidataEntityShort(const string& wdName, const string& readableName, const string& desc, unsigned int nSitelinks = 0)
    : _wdName(wdName), _readableName(readableName), _description(desc), _numSitelinks(nSitelinks) {
    _type = WikidataEntityParse::isPropertyName(_wdName) ? EntityType::Property : EntityType::Subject;}

  // default constructor needed for resize etc
  WikidataEntityShort() = default;

  // TODO: not needed with new JSON library
  //static json
  //nestedVecToArray(const std::vector<std::vector<WikidataEntityShort>> &vec);

  // _____________________________________________________________________________
  // TODO: we are probably not doing the sorting here but in QLever
  static void sortVec(std::vector<std::vector<WikidataEntityShort>>& vec, size_t orderVarIdx, OrderType type, bool asc);

  string toString() {return _wdName + "\t" + _readableName + "\t" + _description;}


  // TODO: should probably not be here
  static int literalToInt(const std::string &str);

};

void to_json(json &j, const WikidataEntityShort &ent);

#endif  // _WIKIDATA_ENTITY_H
