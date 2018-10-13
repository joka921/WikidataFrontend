// Copyright 2018 Johannes Kalmbach
// Author <johannes.kalmbach@gmail.com>
//
#include "QLeverCommunicator.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include "EntityFinder.h"

using json = nlohmann::json;

// __________________________________________________________________________________
QLeverCommunicator::QLeverCommunicator(const std::string& serverAddress,
                                       unsigned int port)
    : _serverAddress(serverAddress),
      _port(port),
      _client([](const std::string& s) { std::cout << s << std::endl; }) {
  _client.InitSession();
}

// _________________________________________________________________________
QLeverCommunicator::~QLeverCommunicator() { _client.CleanupSession(); }

// _________________________________________________________________
std::string QLeverCommunicator::getRawQLeverResponse(const std::string& query) {
  long statusCode;
  std::string res;
  std::string request =
      "http://" + _serverAddress + ":" + std::to_string(_port);
  request += "/?query=" + query;
  _client.GetText(request, res, statusCode);
  return statusCode == 200 ? res : std::string("");
}

// __________________________________________________________________
std::string QLeverCommunicator::replaceEntitiesInQLeverResult(
    const std::string& jsonQlever, const EntityFinder& finder) {
  try {
    std::string res;
    res =
        "{\"status\":\"ERROR\", \"exception\":\"Error while parsing json from "
        "QLever Backend\"}";
    json j;
    std::stringstream stream(jsonQlever);
    stream >> j;
    if (j.at("status") != "OK") return jsonQlever;

    auto& r = j.at("res");
    std::vector<std::vector<WikidataEntity>> entities(r.size());

    size_t idx = 0;
    for (auto& el : r) {
      for (auto& wdNameV : el) {
        std::string wdName = wdNameV;
        entities[idx].push_back(finder.wdNamesToEntities(wdName));
      }
      idx++;
    }
    if (entities.size() > 20) entities.resize(20);
    j["res"] = entities;
    return j.dump();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "trying to return \n";
  }
  return "{\"status\":\"ERROR\", \"exception\":\"Error while parsing json from "
         "QLever Backend\"}";
}

// ____________________________________________________________________
std::string QLeverCommunicator::GetQueryResult(const std::string& query,
                                               const EntityFinder& finder) {
  return replaceEntitiesInQLeverResult(getRawQLeverResponse(query), finder);
}
