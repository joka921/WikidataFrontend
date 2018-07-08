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

#include "./EntityFinder.h"
#include "./ServerUtils.h"
#include "./QLeverCommunicator.h"
#include "./SearchServer.h"


namespace basio = boost::asio;

using std::cout;
using std::endl;




// _____________________________________________________________________________
int main(int argc, char** argv) {
  // Parse the command line arguments.
  if (argc < 3 || argc > 4) {
    std::cerr << "Usage " << argv[0] << " <inputfile> <port>\n";
    exit(1);
  }
  std::string inputFile = argv[1];
  uint16_t port = atoi(argv[2]);

  // createEntityFinder
  EntityFinder finder;
  //QLeverCommunicator communicator("alicudi.informatik.privat", 9998);
  if (argc == 3) {
    finder = EntityFinder::ReadFromFile(std::string(argv[1]) + "preprocessed.dat");
    //finder.InitializeFromTextFile(argv[1]);
    //finder.WriteToFile(std::string(argv[1]) + "preprocessed.dat");
  } else {
    finder = EntityFinder::ReadFromFile("test.dat");
    std::cout << finder.aliasVec.size()<< std::endl;
    std::cout << finder.aliasVec[0].first;
  }
  SearchServer server(std::move(finder), port);
  server.run();
  /*

  // set up server endpoints etc
  basio::io_service ioService;
  basio::ip::tcp::endpoint endpoint(basio::ip::tcp::v4(), port);
  basio::ip::tcp::acceptor acceptor(ioService, endpoint);

  // converter for utf8
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  // set locale to not rely on machine internals
  std::setlocale(LC_ALL, "en_US.utf8");

  // I have used the whitelist approach, because it is safer
  // (I was not sure if it would be sufficient to filter anything
  // starting with "../" or "/"
  std::unordered_set<string> whitelist{
    "search.css", "search.js", "search2.js", "search3.js", "search.html", "js_cookie.js"};

  while (true) {
    try {
      basio::ip::tcp::socket sck(ioService);
      std::cout << "Waiting for query on port "
        << port << " ..." << std::endl;
      acceptor.accept(sck);

      // read request string
      basio::streambuf requestBuf;
      read_until(sck, requestBuf, "\r");
      std::istream istream(&requestBuf);
      std::string req;
      std::getline(istream, req);
      std::cout << "Request string is " << req << std::endl;

      // variables for building HTTP header
      std::string contentString = "";
      std::string contentType = "text/plain";
      std::string contentStatus = "HTTP/1.1 200 OK";

      // this variable turns false if sth goes wrong,
      // so we can send a 404 in the end if necessary
      bool validReq = true;
      auto posReqEnd = req.find(" HTTP/1.1");
      if (req.substr(0, 5) != std::string("GET /") || posReqEnd == req.npos) {
        // beginning or end of request are not valid
        validReq = false;
        contentString = "this server only answers get requests";
      } else {
        // request is valid, cut relevant part
        auto filename = req.substr(5, posReqEnd - 5);
        if (filename.substr(0, 3) == std::string("?t=")) {
          // query request, find matches
          contentType = "application/json";
          auto parsed = ServerUtils::parseQuery(filename);
          //TODO: Implement UTF-8 again
          //auto queryWide = converter.from_bytes(query.c_str());
          
          if (parsed.first.length() == 0 || parsed.second == SearchMode::Invalid) {
            contentString = "[]";
          } else {
            auto res = finder.findEntitiesByPrefix(parsed.first, parsed.second);
           // get at most ten results as JSON
           contentString = ServerUtils::entitiesToJson(res, 10);
          }
        } else if (filename.substr(0, 3) == std::string("?c=")) {
          contentType = "application/json";
          auto listOfNames = ServerUtils::decodeURL(filename.substr(3));
          auto vecOfNames = ServerUtils::split(listOfNames, ' ');
          //TODO: convert ALL
          // and do this directly without detour in client
          // TODO: probably not needed
          //contentString = ServerUtils::entitiesToJson(finder.wdNamesToEntities(vecOfNames), 100);
        } else if (filename.substr(0, 3) == std::string("?s=")) {
	  auto remainder = filename.substr(3);
	  auto pos = remainder.find("?");
	  auto settings = ServerUtils::parseSetting(remainder.substr(0, pos));
	  std::cout << "setting string is valid? " << settings.isValid << std::endl;
	  if (settings.isValid && remainder.substr(pos, 3) == std::string("?r=")) {
	    contentType = "application/json";
	    //auto query = ServerUtils::decodeURL(remainder.substr(pos));
	    auto query = remainder.substr(3);
	    contentString = communicator.GetQueryResult(query, &finder, settings);
	  }

        } else {
        // redirect empty string (start page) to standard file
        if (!filename.length()) filename = "search.html";
        // match file ending
          auto pairContentType = ServerUtils::detectContentType(filename);
          contentType = pairContentType.second;
          if (!pairContentType.first) {
            validReq = false;
            contentString = "unsupported file type detected";
          } else {
            if (whitelist.find(filename) != whitelist.end()) {
              auto fileContents = ServerUtils::readFile(filename);
              if (!fileContents.first) validReq = false;
              contentString = fileContents.second;
            } else {
              contentType = "text/html";
              contentStatus = "HTTP/1.1 403 Forbidden";
              contentString = "this is forbidden you bad person";
            }
          }
        }
      }
      // if sth. went wrong, send a 404 message. the content string
      // has already been set in this case
      if (!validReq) {
      contentStatus = "HTTP/1.1 404 File not found";
      contentType = "text/html";
      }

      // build http response
      std::string response = contentStatus + "\r\n";
      response.append("Content-Length:");
      response.append(std::to_string(contentString.length()));
      response.append("\r\nContent-Type:");
      // if necessary explicityly set encoding to utf8
      contentType.append(";charset=utf-8");
      response.append(contentType);
      response.append("\r\n\r\n");
      response.append(contentString);

      // transmit http response
      //std::cout << response << std::endl;
      boost::system::error_code wErr;
      std::cout << "Peer IP: " << sck.remote_endpoint().address().to_string() << std::endl;
      std::cout << "Peer port: " << sck.remote_endpoint().port() << std::endl;
      basio::write(sck, basio::buffer(response),
          basio::transfer_all(), wErr);
      std::cout << wErr.message() << std::endl;
    } catch (const std::exception& e) {
      std::cerr << e.what() << std::endl;
    }
  }
  */
}
