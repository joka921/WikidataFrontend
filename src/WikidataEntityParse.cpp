#include "WikidataEntityParse.h"

// _____________________________________________________________________
WikidataEntityParse::WikidataEntityParse(const std::string& line) {

  auto pos = line.find("\t");
  _wdName = line.substr(0, pos);
  auto newpos = line.find("\t", pos + 1);
  _numSiteLinks = std::stoi(line.substr(pos + 1, newpos - pos));
  pos = newpos;
  while (newpos != std::string::npos) {
    newpos = line.find("\t", pos + 1);
    auto alias = line.substr(pos + 1, newpos - pos);
    _aliases.push_back(alias);
    pos = newpos;
  }
}

void to_json(json &j, const WikidataEntityShort &ent) {
  j = json();
  j["_wdName"] = ent._wdName;
  j["name"] = ent._readableName;
  j["description"] = ent._description;
  j["type"] = entityTypeToNumeric(ent._type);
}


// _____________________________________________________________________________
void WikidataEntityShort::sortVec(std::vector<std::vector<WikidataEntityShort>>& vec, size_t orderVarIdx, OrderType type, bool asc) {
  // numSitelinks as default case
  // TODO: make absolutely exception safe
  auto sortPredSitelinks = [orderVarIdx] (const std::vector<WikidataEntityShort>& v1, const std::vector <WikidataEntityShort>& v2) 
                                       { return v1[orderVarIdx]._numSitelinks < v2[orderVarIdx]._numSitelinks;};
  auto sortPredNumeric = [orderVarIdx] (const std::vector<WikidataEntityShort>& v1, const std::vector <WikidataEntityShort>& v2) 
                                       { return literalToInt(v1[orderVarIdx]._readableName) < literalToInt(v2[orderVarIdx]._readableName);};
  auto sortPredAlphabetical = [orderVarIdx] (const std::vector<WikidataEntityShort>& v1, const std::vector <WikidataEntityShort>& v2) 
                                       { return v1[orderVarIdx]._readableName < v2[orderVarIdx]._readableName;};

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
  ar(_wdName, name, description, type);
}
*/
