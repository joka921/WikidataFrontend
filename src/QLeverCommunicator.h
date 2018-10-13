// Copyright 2018 Johannes Kalmbach
// Author <johannes.kalmbach@gmail.com>
//
#ifndef _QLEVER_COMMUNICATOR_H
#define _QLEVER_COMMUNICATOR_H
#include <string>
#include <gtest/gtest.h>

#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include "HTTPClient.h"
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

  // Return the result payload (as json) when sending the escaped SPARQL query
  // query query to the QLever server
  std::string getRawQLeverResponse(const std::string& query);

  // in a json result from QLever replace the entries by WikidataEntity entries.
  // Whenever possible we will replace abstract URIs like <http://wikidata.org/entities/Q42>
  // by information stored in the EntityFinder * finder
  std::string replaceEntitiesInQLeverResult(const std::string &jsonFromQLever, const EntityFinder &finder);
  FRIEND_TEST(QLeverCommunicatorTest, replaceEntitiesInQLeverResult);

  public:
  // Constructor
    QLeverCommunicator(const std::string& serverAddress, unsigned int port);

  // Destructor
    ~QLeverCommunicator();

    // public interface: send a SPARQL query (argument query) to the QLever backend
    // and replace the abstract entity URIs in the result by information from
    // the EntityFinder wherever this is possible
    std::string GetQueryResult(const std::string &query, const EntityFinder &finder);
};

#endif  // _QLEVER_COMMUNICATOR_H
