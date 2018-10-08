#ifndef _QLEVER_COMMUNICATOR_H
#define _QLEVER_COMMUNICATOR_H
#include <string> 

#include "HTTPClient.h"
#include "QLeverResult.h"
#include "ServerUtils.h"

// cyclic dependency
class EntityFinder;

// Class which handles the communication from this Entity Finder / Frontend
// Server to the QLever Backend.
class QLeverCommunicator {
  // address of QLever server
  std::string _serverAddress;
  // port of the QLever server
  unsigned int _port;
  // http client handling the request
  CHTTPClient _client;

  // Return the result payload (json) when sending the escaped SPARQL query
  // query query to the QLever server
  std::string getRawQLeverResponse(const std::string& query);
  std::string parseJSON(const std::string& json, const EntityFinder* finder, const QuerySettings& settings);

  public:
    QLeverCommunicator(const std::string& serverAddress, unsigned int port);
    ~QLeverCommunicator();
    std::string GetQueryResult(const std::string& query, const EntityFinder* finder, const QuerySettings& settings);
};

#endif  // _QLEVER_COMMUNICATOR_H
