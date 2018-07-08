
// Copyright 2017, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claudius Korzen <korzen@cs.uni-freiburg.de>.

#ifndef SEARCHSERVER_H_
#define SEARCHSERVER_H_

#include <boost/asio.hpp>
#include <regex>
#include <codecvt>
#include <unordered_set>
#include "EntityFinder.h"
#include "QLeverCommunicator.h"


// The default HTTP response header.
const char HTTP_OK_HEADER[] = "HTTP/1.1 200 OK";

// The default content type to return in HTTP responses.
const char DEFAULT_CONTENT_TYPE[] = "text/plain";

// The default line delimiter.
const char DEFAULT_LINE_DELIMITER[] = "\r\n";

// The available content types, per file extensions.
const std::map<std::string, std::string> CONTENT_TYPES = {
  { ".html", "text/html" },
  { ".css", "text/css" },
  { ".js", "application/javascript" },
  { ".json", "application/json" },
  { ".jpg", "image/jpeg" },
  { ".png", "image/png" }
};

// The available HTTP error responses by status codes.
const std::map<uint16_t, std::string> HTTP_ERROR_RESPONSES = {
  { 403, "HTTP/1.1 403 Forbidden\r\n"
         "Content-type: text/html\r\n"
         "Content-length: 10\r\n\r\n"
         "Forbidden." },
  { 404, "HTTP/1.1 404 Not found\r\n"
         "Content-type: text/html\r\n"
         "Content-length: 10\r\n\r\n"
         "Not found."},
  { 405, "HTTP/1.1 405 Method Not Allowed\r\n"
         "Content-type: text/html\r\n"
         "Content-length: 12\r\n\r\n"
         "Not allowed." }
};

// The regex pattern of the first line from a valid HTTP request header.
const std::regex HTTP_REQUEST_HEADER_REGEX(
    "([A-Z]+) /([a-zA-Z0-9.-]*)(\\?\\S*)? HTTP/1.1");

// The default encoding.
const char DEFAULT_ENCODING[] = "UTF-8";

// The default file to serve if no fileName is given in the HTTP request.
const char DEFAULT_FILE_NAME[] = "search.html";

// The URL to the default image of an entity, if no image url is given.
const char DEFAULT_ENTITY_IMAGE_URL[] = "noimage.png";

// The number of search results to show per default.
const size_t NUM_ENTITIES_TO_SHOW = 5;


// A simple server that handles fuzzy prefix search requests and file requests.
class SearchServer {
 public:
  // Creates a new search server.
  SearchServer(EntityFinder&& finder,  uint16_t port) :
        _finder(std::move(finder)),
	_communicator("alicudi.informatik.privat", 9999),
        _server(boost::asio::ip::tcp::v4(), port),
        _acceptor(_ioService, _server),
        _client(_ioService),
        _timer(_ioService),
        _whitelist{
      "search.css", "search.js", "search2.js", "search3.js", "search.html", "js_cookie.js"}
        {}

  // Starts the server loop.
  void run();

 private:
  // Handles a timeout of the client.
  void handleTimeout(const boost::system::error_code& error);

  // Handles a HTTP request.
  void handleRequest(const boost::system::error_code& error);

  // Creates the HTTP response for the given HTTP request.
  std::string createResponse(const std::string& request) ;

  // Handles a fuzzy prefix search request for the query given in 'params', and
  // plugs the result into the given stream (that holds the content of
  // search.html).
  std::stringstream handleFuzzyPrefixSearchRequest(const std::string& params)
    const;


  // Returns the content type of the given file.
  static std::string getContentType(const std::string& fileName);

  EntityFinder _finder;
  QLeverCommunicator _communicator;
  std::unordered_set<string> _whitelist;

  // The server socket.
  boost::asio::ip::tcp::endpoint _server;

  // The service that handles the I/O functionality.
  boost::asio::io_service _ioService;

  // The acceptor.
  boost::asio::ip::tcp::acceptor _acceptor;

  // The client socket.
  boost::asio::ip::tcp::socket _client;

  // The timeout timer.
  boost::asio::deadline_timer _timer;

  // The request buffer.
  boost::asio::streambuf _requestBuffer;
};

#endif  // SEARCHSERVER_H_
