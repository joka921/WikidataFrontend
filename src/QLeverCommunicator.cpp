#include "QLeverCommunicator.h"
#include <sstream>
#include <nlohmann/json.hpp>
#include "EntityFinder.h"

using json = nlohmann::json;

// __________________________________________________________________________________
QLeverCommunicator::QLeverCommunicator(const std::string& serverAddress, unsigned int port)
    :  _serverAddress(serverAddress), _port(port), _client([](const std::string& s) {std::cout << s << std::endl;}) {
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
std::string QLeverCommunicator::parseJSON(const std::string &jsonQlever,
                                          const EntityFinder *finder) {
  std::string res;
  res = "{\"status\":\"ERROR\", \"exception\":\"Error while parsing json from QLever Backend\"}";
  json j;
  std::stringstream stream(jsonQlever);
  stream >> j;
  std::string statusIt = j.at("status");
  if (j.at("status") != "OK")
    return jsonQlever;

  auto &r = j.at("res");
  std::vector<std::vector<WikidataEntity>> entities(r.size());
  
  size_t idx = 0;
  for (auto& el : r) {
    for (auto &wdNameV : el) {
      std::string wdName = wdNameV;
      entities[idx].push_back(finder->wdNamesToEntities(wdName));
    }
    idx++;
  }
  if (entities.size() > 20) entities.resize(20);
  j["res"] = entities;
  return j.dump();
}


// ____________________________________________________________________
std::string QLeverCommunicator::GetQueryResult(const std::string& query, const EntityFinder* finder) {
  return parseJSON(getRawQLeverResponse(query), finder);
}
