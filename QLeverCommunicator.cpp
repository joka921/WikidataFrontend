#include "QLeverCommunicator.h"

#include <sstream>

#include "./picojson/picojson.h"
#include "EntityFinder.h"

// __________________________________________________________________________________
QLeverCommunicator::QLeverCommunicator(const std::string& serverAddress, unsigned int port)
    : _port(port), _serverAddress(serverAddress), _client([](const std::string& s) {std::cout << s << std::endl;}) {
    _client.InitSession();
}

// _________________________________________________________________________
QLeverCommunicator::~QLeverCommunicator() {
  _client.CleanupSession();
}

// _________________________________________________________________
std::string QLeverCommunicator::getRawQLeverResponse(const std::string& query) {
  long statusCode;
  std::string res;
  std::string request ="http://" + _serverAddress + ":" + std::to_string(_port);  
  request += "/?query=" + query;
  _client.GetText(request, res, statusCode);
  return statusCode == 200 ? res : std::string("");
}

// __________________________________________________________________
std::string QLeverCommunicator::parseJSON(const std::string& json, const EntityFinder* finder, const QuerySettings& settings) {
  std::string res;
  //TODO:: default must be proper json response with error code etc.
  res = "{\"status\":\"ERROR\", \"exception\":\"Error while parsing json from QLever Backend\"}";
  picojson::value v;
  std::stringstream stream(json);
  stream >> v;
  if (! v.is<picojson::object>()) {
    return res;
  }
  auto& m = v.get<picojson::object>();
  auto statusIt = m.find("status");
  if (statusIt == m.end()) return res;
  if (! statusIt->second.is<std::string>()) return res;
  const auto& status = statusIt->second.get<std::string>();
  if (status != "OK") return json;

  // handle ordering
  /*
  auto selectedIt = m.find("selected");
  if (selectedIt != m.end()) {
    if (! selectedIt->second.is<picojson::array>()) return res;
      auto& sel= selectedIt->second.get<picojson::array>();
      for (int i = 0; i < sel.size(); ++i) {
	if (! sel[i].is<std::string>()) break;
	const auto& varName = sel[i].get<std::string>();
	if (varName == orderVariable) {
	  isOrderVarFound = true;
	  orderIdx = i;
	  break;
	}
      }
  }
  auto sortPredPopularity = [orderIdx] (const std::vector<WikidataEntityShort>& v1, const std::vector <WikidataEntityShort>& v2) 
                                       { return v1[orderIdx].numSitelinks < v2[orderIdx].numSitelinks;};
  */

  auto it = m.find("res");
  if (it == m.end()) return res;

  if (! it->second.is<picojson::array>()) return res;
  auto& r = it->second.get<picojson::array>();

  // TODO: this is really bad. We only truncate here for the demo
  // TODO TODO TODO handle this better (save results and return them on request)

  // r is now the vector which hold one result vector each
  // first store all the entities to make sorting easier
  std::vector<std::vector<WikidataEntityShort>> entities(r.size());
  
  size_t idx = 0;
  for (auto& el : r) {
    if (! el.is<picojson::array>()) return res;
    auto& single = el.get<picojson::array>();
    for (auto& wdNameV : single) {
      if (!wdNameV.is<std::string>()) return res;
      const auto& wdName = wdNameV.get<std::string>();
      entities[idx].push_back(finder->wdNamesToEntities(wdName));
      //wdNameV.set(entity.ConvertToPicojsonObject());
    }
    idx++;
  }
  if (settings.type != OrderType::None && entities.size() && settings.orderIdx <entities[0].size()) {
    //std::sort(entities.rbegin(), entities.rend(), sortPredPopularity);
    WikidataEntityShort::sortVec(entities, settings.orderIdx, settings.type, settings.asc);
  }
  if (entities.size() > 20) entities.resize(20);
  it->second.set(WikidataEntityShort::nestedVecToArray(entities));
  return v.serialize();
}


// ____________________________________________________________________
std::string QLeverCommunicator::GetQueryResult(const std::string& query, const EntityFinder* finder, const QuerySettings& settings) {
  return parseJSON(getRawQLeverResponse(query), finder, settings);
}
