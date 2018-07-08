#ifndef _QLEVER_COMMUNICATOR_H
#define _QLEVER_COMMUNICATOR_H
// Class which handles the communication from this Frontend Server to the QLever
// Backend.
//
#include <string> 

#include "HTTPClient.h"
#include "QLeverResult.h"
#include "ServerUtils.h"

// cyclic dependency
class EntityFinder;



// ________________________________________________________________________--
class QLeverCommunicator {
  std::string _serverAddress;
  unsigned int _port;
  CHTTPClient _client;

  std::string getRawQLeverResponse(const std::string& query);
  std::string parseJSON(const std::string& json, const EntityFinder* finder, const QuerySettings& settings);

  public:
    QLeverCommunicator(const std::string& serverAddress, unsigned int port);
    ~QLeverCommunicator();
    std::string GetQueryResult(const std::string& query, const EntityFinder* finder, const QuerySettings& settings);
};

#endif  // _QLEVER_COMMUNICATOR_H
