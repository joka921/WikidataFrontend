// Copyright 2016 Johannes Kalmbach
// Authors: <johannes.kalmbach@gmail.com>
//

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <string.h>
#include <utility>
#include <locale>
#include <codecvt>
#include <unordered_set>

#include "./src/EntityFinder.h"
#include "./src/QLeverCommunicator.h"
#include "./src/SearchServer.h"
#include "./src/ServerUtils.h"

namespace basio = boost::asio;

using std::cout;
using std::endl;

// _____________________________________________________________________________
int main(int argc, char** argv) {
  // Parse the command line arguments.
  if (argc != 3) {
    std::cerr << "Usage " << argv[0] << " <inputfile> <port>\n";
    exit(1);
  }
  std::string inputFile = argv[1];
  uint16_t port = atoi(argv[2]);

  auto finder = EntityFinder::SetupFromFilename(inputFile);
  SearchServer server(std::move(finder), port);
  server.run();
}
