// Copyright 2018 Johannes Kalmbach
// Author <johannes.kalmbach@gmail.com>
//
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

#include "./SearchServer.h"
//#include "./Util.h"

// _____________________________________________________________________________
void SearchServer::run() {
  // The main server loop.
  while (true) {
    try {
      // Wait for the client.
      std::cout << "Waiting on port " << _server.port() << " ... "
                << std::flush;
      // Make sure that _client is closed, so we get no already open exception
      _client.close();
      _acceptor.accept(_client);
      std::cout << "client connected from "
                << _client.remote_endpoint().address().to_string() << std::endl;

      try {
        // Set the timeout for the client.
        _timer.expires_from_now(boost::posix_time::millisec(100));
        _timer.async_wait(boost::bind(&SearchServer::handleTimeout, this, _1));

        // Read only the first line of the request from the client.
        boost::asio::async_read_until(
            _client, _requestBuffer, DEFAULT_LINE_DELIMITER,
            boost::bind(&SearchServer::handleRequest, this, _1));

        _ioService.run();
        _ioService.reset();
      } catch (boost::exception& e) {
        // Socket was close by remote, continue listening
        std::cout << "WARN: " << boost::diagnostic_information(e) << std::endl;
        continue;
      }
    } catch (const std::exception& e) {
      std::cerr << e.what() << std::endl;
    }
  }
}

// _____________________________________________________________________________
void SearchServer::handleTimeout(const boost::system::error_code& e) {
  if (e) {
    return;
  }

  std::cout << "Timeout. " << std::endl;

  // Clear the request buffer.
  _requestBuffer.consume(_requestBuffer.size());

  // Cancel the timeout timer.
  _timer.cancel();

  // Close the connection to the client.
  _client.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
  _client.close();
}

// _____________________________________________________________________________
void SearchServer::handleRequest(const boost::system::error_code& e) {
  if (e) {
    return;
  }

  std::string request;
  std::istream istream(&_requestBuffer);
  std::getline(istream, request);

  // IMPORTANT NOTE: read_until() seems to actually read more than up to the
  // delimiter '\r\n'. So check if the reqString still contains the delimiter
  // and remove it.
  size_t pos1 = request.find("\r\n");
  if (pos1 != std::string::npos) {
    request = request.replace(pos1, 2, "");
  }
  size_t pos2 = request.find("\r");
  if (pos2 != std::string::npos) {
    request = request.replace(pos2, 1, "");
  }

  // Handle the request line and create the response.
  std::string response = createResponse(request);

  // Send the response to the client.
  boost::asio::write(_client, boost::asio::buffer(response),
                     boost::asio::transfer_all());

  // Clear the request buffer.
  _requestBuffer.consume(_requestBuffer.size());

  // Cancel the timeout timer.
  _timer.cancel();

  // Close the connection to the client.
  _client.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
  _client.close();
}

// _____________________________________________________________________________
std::string SearchServer::createResponse(const std::string& req) {
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
      auto parsed = ServerUtils::parsePrefixSearchQuery(filename);
      // TODO: Implement UTF-8 again
      // auto queryWide = converter.from_bytes(query.c_str());

      if (parsed.first.length() == 0 || parsed.second == SearchMode::Invalid) {
        contentString = "[]";
      } else {
        auto res = _finder.findEntitiesByPrefix(parsed.first, parsed.second);
        // get at most ten results as JSON
        contentString = ServerUtils::entitiesToJson(res);
      }
    } else if (filename.substr(0, 3) == std::string("?c=")) {
      contentType = "application/json";
      std::cout << filename.substr(3) << '\n';
      auto listOfNames = ServerUtils::decodeURL(filename.substr(3));
      std::cout << listOfNames << '\n';
      auto vecOfNames = ServerUtils::split(listOfNames, ' ');
      contentString =
          ServerUtils::entitiesToJson(_finder.wdNamesToEntities(vecOfNames));
    } else if (filename.substr(0, 3) == std::string("?r=")) {
      contentType = "application/json";
      auto query = filename.substr(3);
      contentString = _communicator.GetQueryResult(query, _finder);
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
        if (_whitelist.find(filename) != _whitelist.end()) {
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

  return response;
}

// _____________________________________________________________________________
std::string SearchServer::getContentType(const std::string& uri) {
  std::string contentType = DEFAULT_CONTENT_TYPE;

  // Obtain the file extension of the given file.
  std::string fileExtension = "";
  size_t pos = uri.find_last_of(".");
  if (pos != std::string::npos) {
    fileExtension = uri.substr(pos);
  }

  // Look up the content type for the file extension.
  if (CONTENT_TYPES.find(fileExtension) != CONTENT_TYPES.end()) {
    contentType = CONTENT_TYPES.find(fileExtension)->second;
  }

  return contentType;
}
