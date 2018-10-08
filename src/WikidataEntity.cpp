#include "WikidataEntity.h"

// _____________________________________________________________________
WikidataEntity::WikidataEntity(const std::string& line) {

  auto pos = line.find("\t");
  name = line.substr(0, pos);
  auto newpos = line.find("\t", pos + 1);
  numSitelinks = std::stoi(line.substr(pos + 1, newpos - pos));
  pos = newpos;
  while (newpos != std::string::npos) {
    newpos = line.find("\t", pos + 1);
    auto alias = line.substr(pos + 1, newpos - pos);
    aliases.push_back(alias);
    pos = newpos;
  }
}

void to_json(json &j, const WikidataEntityShort &ent) {
  j = json();
  j["wdName"] = ent.wdName;
  j["name"] = ent.name;
  j["description"] = ent.description;
  j["type"] = EntityTypeToString(ent.type);
}

// ____________________________________________________
json WikidataEntityShort::nestedVecToArray(
    const std::vector<std::vector<WikidataEntityShort>> &vec) {
  return vec;
}

// Converter Function
std::string EntityTypeToString(const EntityType& type) {
  if (type == EntityType::Subject) {
    return "0";
  }
  return "1";
}

// _____________________________________________________________________________
void WikidataEntityShort::sortVec(std::vector<std::vector<WikidataEntityShort>>& vec, size_t orderVarIdx, OrderType type, bool asc) {
  // numSitelinks as default case
  // TODO: make absolutely exception safe
  auto sortPredSitelinks = [orderVarIdx] (const std::vector<WikidataEntityShort>& v1, const std::vector <WikidataEntityShort>& v2) 
                                       { return v1[orderVarIdx].numSitelinks < v2[orderVarIdx].numSitelinks;};
  auto sortPredNumeric = [orderVarIdx] (const std::vector<WikidataEntityShort>& v1, const std::vector <WikidataEntityShort>& v2) 
                                       { return literalToInt(v1[orderVarIdx].name) < literalToInt(v2[orderVarIdx].name);};
  auto sortPredAlphabetical = [orderVarIdx] (const std::vector<WikidataEntityShort>& v1, const std::vector <WikidataEntityShort>& v2) 
                                       { return v1[orderVarIdx].name < v2[orderVarIdx].name;};

  if (type == OrderType::NumSitelinks) {
    if (asc) {
      std::sort(vec.begin(), vec.end(), sortPredSitelinks);
    } else {
      std::sort(vec.rbegin(), vec.rend(), sortPredSitelinks);
    }
  } else if (type == OrderType::Numeric) {
    if (asc) {
      std::sort(vec.begin(), vec.end(), sortPredNumeric);
    } else {
      std::sort(vec.rbegin(), vec.rend(), sortPredNumeric);
    }
  } else if (type == OrderType::Alphabetical) {
    if (asc) {
      std::sort(vec.begin(), vec.end(), sortPredAlphabetical);
    } else {
      std::sort(vec.rbegin(), vec.rend(), sortPredAlphabetical);
    }
  }


}

// ____________________________________________________________________________
int WikidataEntityShort::literalToInt(const std::string& str) {
  auto sub = str.substr(1, str.size() - 2);
  int result = 0;
  try {
    result = std::stoi(sub);
   } catch (std::invalid_argument&) {
     return 0;
   } catch (std::out_of_range&) {
     return 0;
   }
  return result;
}

// ____________________________________________________________________--
/*
template<class Archive>
void WikidataEntityShort::serialize(Archive& ar, std::uint32_t const version) {
  ar(wdName, name, description, type);
}
*/
