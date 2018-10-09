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
  // (see REAME for file type description)
  WikidataEntityParse(const std::string& line) {
    // everything until the first tab is the wikidata name
    auto pos = line.find("\t");
    _wdName = line.substr(0, pos);

    // until the next tab we have the number of sitelinks
    auto newpos = line.find("\t", pos + 1);
    _numSiteLinks = std::stoi(line.substr(pos + 1, newpos - pos));
    pos = newpos;

    // the rest are aliases (also tab-separated)
    while (newpos != std::string::npos) {
      newpos = line.find("\t", pos + 1);
      auto alias = line.substr(pos + 1, newpos - pos);
      _aliases.push_back(alias);
      pos = newpos;
    }
  }

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


  // TODO: should probably not be here
  static int literalToInt(const std::string &str);

};

// convert a WikidataEntityShort to JSON
inline void to_json(json &j, const WikidataEntityShort &ent) {
  j = json();
  j["wdName"] = ent._wdName;
  j["name"] = ent._readableName;
  j["description"] = ent._description;
  j["type"] = entityTypeToNumeric(ent._type);
  j["numSitelinks"] = ent._numSitelinks;
}

#endif  // _WIKIDATA_ENTITY_H
