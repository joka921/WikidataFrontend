// Copyright 2018 Johannes Kalmbach
// Author <johannes.kalmbach@gmail.com>
//
#ifndef _SERVER_UTILS_H
#define _SERVER_UTILS_H

#include <string>
#include <utility>
#include <vector>

#include "EntityFinder.h"
#include "EntityFinderHelpers.h"
#include "WikidataEntityParse.h"

class ServerUtils {
 public:
  // Trivial helper functions
  // Serialize the input arguments to json. Format will be res["entities"] =
  // <json version of input>
  static string entitiesToJson(
      const std::vector<std::vector<WikidataEntity>>& entities);
  static string entitiesToJson(const std::vector<WikidataEntity>& entities);

  // parse a Prefix Search query that has the form "?t=obj?q=<prefix>",
  // "?t=prd?q=<prefix>" or "?t=all?q=<prefix>" <prefix> lands in the string of
  // the result type and SearchMode is set according to "obj, prd, all".
  // Ill-formed queries will cause SearchMode::Invalid and an undefined result
  // string.
  static std::pair<std::string, SearchMode> parsePrefixSearchQuery(
      const std::string& query);

  // perform URL decoding on the input argument and return the result
  static std::string decodeURL(std::string encoded);

  // read the file specified by filename arg and return a
  // pair<bool, string> with
  // <successful?, contentsOfFile>
  static std::pair<bool, std::string> readFile(std::string filename);
  // detect the file ending of the given file name and return the appropriate
  // MIME type. only html, css and js are correct.
  // Returns: pair<bool, string> with <successful?, MIME-type>
  static std::pair<bool, std::string> detectContentType(std::string filename);

  // A simple function for splitting strings at a given delimiter
  static std::vector<std::string> split(const std::string& s, char delim);
};

#endif  // _SERVER_UTILS_H
