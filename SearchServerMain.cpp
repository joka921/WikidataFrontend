// Copyright 2016 Johannes Kalmbach
// Authors: <johannes.kalmbach@gmail.com>
//

#include <iostream>
#include <memory>
#include <string.h>
#include <string>

#include "./src/EntityFinder.h"
#include "./src/SearchServer.h"

// _____________________________________________________________________________
int main(int argc, char** argv) {
  // Parse the command line arguments.
  if (argc != 3 && argc != 5) {
    std::cerr
        << "Usage " << argv[0]
        << " <inputfile-prefix> <port> (<ip-of-qlever> <port-of-qlever)\n";
    exit(1);
  }
  std::string inputFile = argv[1];
  uint16_t port = atoi(argv[2]);

  auto finder = EntityFinder::SetupFromFilePrefix(inputFile);
  std::unique_ptr<SearchServer> server(nullptr);
  if (argc == 3) {
    server.reset(new SearchServer(std::move(finder), port));
  } else {
    std::string qleverIp = argv[3];
    uint16_t qleverPort = atoi(argv[4]);
    server.reset(
        new SearchServer(std::move(finder), port, qleverIp, qleverPort));
  }
  server->run();
}
