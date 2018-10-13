// Copyright 2018 Johannes Kalmbach
// Author <johannes.kalmbach@gmail.com>
//

#include "EntityFinder.h"

#include <chrono>
#include <fstream>
#include <ios>
#include <sstream>

// ______________________________________________________________________
void EntityFinder::InitializeFromTextFile(const std::string& filePrefix) {
  // Open the entity and the description file
  std::ifstream file(filePrefix + ".entities");
  std::string descriptionFilename = filePrefix + ".desc";
  std::ifstream fileDesc(descriptionFilename);
  if (!(file && fileDesc)) {
    std::cerr
        << "ERROR: either the name file " << filePrefix
        << " or the description filePrefix " << descriptionFilename
        << "could not been opened. Search engine will contain no entities.\n";
    return;
  }

  // Read and parse the files
  std::string line;
  while (std::getline(file, line)) {
    auto entity = WikidataEntityParse(line);
    // Determine whether this is a property (P... or a entity (Q...)
    // and determine the correct data structures to store the data
    auto& v = WikidataEntityParse::isPropertyName(entity._wdName)
                  ? _propertyVecs
                  : _subjectVecs;
    auto entityShort = WikidataEntity(entity);

    // read description for this entity (next line in description file)
    std::string tempDesc;
    if (!std::getline(fileDesc, tempDesc)) {
      tempDesc = "no description";
    }
    entityShort._description = tempDesc;

    // if we have read an ill-formed entity we skip it here. We still had to
    // read one line from the description file
    if (!entity._isValid) {
      continue;
    }
    v._entities.push_back(std::move(entityShort));

    // push all aliases of the entity and the index of the current entity
    for (auto& el : entity._aliases) {
      std::transform(el.begin(), el.end(), el.begin(), ::tolower);
      v._aliases.emplace_back(el, v._entities.size() - 1);
    }
  }

  // sort the aliases alphabetically so we can perform binary search on them
  auto sortPred = [](const std::pair<std::string, size_t>& p1,
                     const std::pair<std::string, size_t>& p2) {
    return p1.first < p2.first;
  };
  std::sort(_subjectVecs._aliases.begin(), _subjectVecs._aliases.end(),
            sortPred);
  std::sort(_propertyVecs._aliases.begin(), _propertyVecs._aliases.end(),
            sortPred);

  // Set up the "translation indices"
  std::array<EntityVectors*, 2> vecs = {&_subjectVecs, &_propertyVecs};
  for (auto vPtr : vecs) {
    auto& v = *vPtr;
    size_t num = 0;
    for (const auto& el : v._entities) {
      const auto& wd = el._wdName;
      size_t idx = getIdxFromWdName(wd).first;
      v._entityToIdx[idx] = num;
      num += 1;
    }
    v.shrink_to_fit();
  }
}

// __________________________________________________________________
std::vector<WikidataEntity> EntityFinder::findEntitiesByPrefix(
    string prefix, SearchMode mode) const {
  std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);

  // predicate for lower bound
  auto boundPred = [](const std::pair<std::string, unsigned>& p1,
                      const std::string& p2) { return p1.first < p2; };
  // predicate for the upper bound of all prefix matches
  auto upperBoundPrefixesPred = [](const std::string& p1,
                                   const std::pair<std::string, unsigned>& p2) {
    return p1 < p2.first.substr(0, std::min(p2.first.size(), p1.size()));
  };

  // choose the correct set of internal data structures depending on
  // the search type (Properties or other Entities)
  const auto& v = mode == SearchMode::Properties ? _propertyVecs : _subjectVecs;

  // first find matches for the prefixes
  // lower bound
  auto lower =
      std::lower_bound(v._aliases.begin(), v._aliases.end(), prefix, boundPred);
  if (lower == v._aliases.end() ||
      !std::equal(prefix.begin(), prefix.end(), (*lower).first.begin())) {
    // empty result
    return {};
  }

  auto upperPrefixes =
      std::upper_bound(lower, v._aliases.end(), prefix, upperBoundPrefixesPred);

  // upper bound for the exact matches
  auto upperExact =
      std::lower_bound(lower, upperPrefixes, prefix + ' ', boundPred);
  if (upperExact < upperPrefixes && upperExact->first == prefix) {
    upperExact++;
  }
  size_t numPrefixMatches = upperPrefixes - lower;
  size_t numExactMatches = upperExact - lower;

  // If we get more results than this we do not calculate the exact ranking but
  // truncate the result alphabetically
  const size_t maxRelevant = 100000;

  if (numPrefixMatches > maxRelevant) {
    upperPrefixes = lower + maxRelevant;
    if (numExactMatches > maxRelevant) {
      upperExact = lower + maxRelevant;
    }
  }
  // calculate the ranking
  auto sortedIndices = rankResults(lower, upperExact, upperPrefixes, v);

  // Convert the ranking back to WikidataEnities
  std::vector<WikidataEntity> ret = convertIdxVecsToSearchResult(
      sortedIndices.first, sortedIndices.second, v);
  return ret;
}

// ______________________________________________________________________
std::vector<WikidataEntity> EntityFinder::convertIdxVecsToSearchResult(
    const IdxVec& exactIndices, const IdxVec& prefixIndices,
    const EntityVectors& v) const {
  std::vector<WikidataEntity> ret;
  ret.reserve(_resultsToSend);
  // first the exact matches
  for (size_t i = 0; i < _resultsToSend && i < exactIndices.size(); ++i) {
    auto idx = exactIndices[i]._idx;
    ret.push_back(v._entities[idx]);
  }
  // fill the rest with prefix matches
  for (size_t i = 0;
       i + exactIndices.size() < _resultsToSend && i < prefixIndices.size();
       ++i) {
    auto idx = prefixIndices[i]._idx;
    ret.push_back(v._entities[idx]);
  }
  return ret;
}

// _______________________________________________________________________
std::pair<EntityFinder::IdxVec, EntityFinder::IdxVec> EntityFinder::rankResults(
    AliasIt lower, AliasIt upperExact, AliasIt upperPrefixes,
    const EntityVectors& v) const {
  // store <numberOfSitelinks, idx> pairs for exact
  // and prefix matches to make  sorting faster
  IdxVec onlyIdxVec;
  IdxVec onlyIdxVecExact;
  onlyIdxVec.reserve(upperPrefixes - upperExact);
  onlyIdxVecExact.reserve(upperExact - lower);
  const auto& ent = v._entities;

  // setup sitelink/idx vector for exact matches
  while (lower != upperExact) {
    auto idx = (*lower).second;
    onlyIdxVecExact.emplace_back(ent[idx]._numSitelinks, idx);
    lower++;
  }
  // .. and for prefix matches
  while (lower != upperPrefixes) {
    auto idx = (*lower).second;
    onlyIdxVec.emplace_back(ent[idx]._numSitelinks, idx);
    lower++;
  }

  // predicate for sorting according to the sitelink score (higher score first)
  auto sortPred = [](const IdxVecEntry& w1, const IdxVecEntry& w2) {
    return w1._numSitelinks != w2._numSitelinks
               ? w1._numSitelinks > w2._numSitelinks
               : w1._idx > w2._idx;
  };

  // predicate for the elimination of duplicates
  auto equalPred = [](const IdxVecEntry& w1, const IdxVecEntry& w2) {
    return w1._idx == w2._idx;
  };

  std::sort(onlyIdxVec.begin(), onlyIdxVec.end(), sortPred);
  std::sort(onlyIdxVecExact.begin(), onlyIdxVecExact.end(), sortPred);

  // Eliminate duplicates
  onlyIdxVec.erase(std::unique(onlyIdxVec.begin(), onlyIdxVec.end(), equalPred),
                   onlyIdxVec.end());
  onlyIdxVecExact.erase(
      std::unique(onlyIdxVecExact.begin(), onlyIdxVecExact.end(), equalPred),
      onlyIdxVecExact.end());
  return {std::move(onlyIdxVecExact), std::move(onlyIdxVec)};
}

// ________________________________________________________________________
std::pair<size_t, EntityType> EntityFinder::getIdxFromWdName(
    const std::string& wdName) {
  EntityType type = EntityType::Subject;

  if (wdName.size() < 3 || wdName[0] != '<') {
    // this is not a valid Wikidata entity!
    return std::make_pair(size_t(-1), type);
  }

  if (wdName[1] == 'P') {
    type = EntityType::Property;
  } else if (wdName[1] == 'Q') {
    type = EntityType::Subject;
  } else {
    return std::make_pair(size_t(-1), type);
  }
  std::stringstream s(wdName.substr(2));
  size_t idx;
  s >> idx;
  return std::make_pair(idx, type);
}

// _______________________________________________________________________________
std::vector<std::vector<WikidataEntity>> EntityFinder::wdNamesToEntities(
    const std::vector<std::vector<string>>& wdNames) const {
  std::vector<std::vector<WikidataEntity>> res;
  for (const auto& vec : wdNames) {
    res.push_back(wdNamesToEntities(vec));
  }
  return res;
}
// ___________________________________________________________________
std::vector<WikidataEntity> EntityFinder::wdNamesToEntities(
    const std::vector<string>& wdNames) const {
  std::vector<WikidataEntity> ret;
  for (const auto& el : wdNames) {
    ret.push_back(wdNamesToEntities(el));
  }
  return ret;
}

// ________________________________________________________________________________
WikidataEntity EntityFinder::wdNamesToEntities(const std::string& in) const {
  auto el = ExtractWikidataIdFromUri(in);
  auto p = getIdxFromWdName(el);
  auto& idx = p.first;
  const auto& v =
      p.second == EntityType::Property ? _propertyVecs : _subjectVecs;

  // if we find the entity in our data structure, return its information
  if (v._entityToIdx.count(idx)) {
    idx = v._entityToIdx.at(idx);
    if (idx <= v._entities.size()) {
      return v._entities[idx];
    }
  }

  // no corresponding entity found,
  // return dummy that only has a readable name
  return WikidataEntity("", el, "", 0);
}

// ______________________________________________________________________________
void EntityFinder::SerializeToFile(const std::string& filename) const {
  std::cout << "Writing to file " << filename << std::endl;
  std::ofstream os(filename, std::ios::binary);
  if (os) {
    boost::archive::binary_oarchive oar(os);
    oar << *this;
    std::cout << "Done." << std::endl;
  } else {
    std::cout << "Warning: could not write binary restart file " << filename
              << "\n";
  }
}

// ______________________________________________________________________________
EntityFinder EntityFinder::ReadFromSerializedFile(const std::string& filename) {
  std::cout << "Reading from file " << filename << std::endl;
  std::ifstream os(filename, std::ios::binary);
  boost::archive::binary_iarchive oar(os);
  EntityFinder ent;
  oar >> ent;
  std::cout << "Done." << std::endl;
  return ent;
}

// _____________________________________________________________________________
EntityFinder EntityFinder::SetupFromFilePrefix(const std::string& prefix) {
  std::string filenamePreprocessed = prefix + ".preprocessed.dat";
  std::cout << "Checking if preprocessed file exists\n";
  std::ifstream is(filenamePreprocessed, std::ios::binary);
  if (is) {
    is.close();
    std::cout
        << "Reading from preprocessed file " << filenamePreprocessed
        << ".\nIf this is not desired (e.g. after updating the input data),\n"
           "please rename or remove this file\n";
    return ReadFromSerializedFile(filenamePreprocessed);
  } else {
    std::cout << "no preprocessed file found, reading from original file "
              << prefix << ".\n";
    EntityFinder finder;
    finder.InitializeFromTextFile(prefix);
    finder.SerializeToFile(filenamePreprocessed);
    return std::move(finder);
  }
}

std::string EntityFinder::ExtractWikidataIdFromUri(const string& uri) {
  // entity has to start with <http...
  static const std::string wd = "<http://www.wikidata.org/";
  if (uri.size() < wd.size() ||
      !std::equal(wd.begin(), wd.end(), uri.begin())) {
    return uri;
  }

  auto pos = uri.rfind('/');
  // must be "/Qxxx>" where xxx are digits
  if (pos > uri.size() - 4) {
    return uri;
  }

  if (uri[pos + 1] != 'Q' && uri[pos + 1] != 'P') {
    return uri;
  }

  if (!std::all_of(uri.begin() + pos + 2, uri.end() - 1, ::isdigit)) {
    return uri;
  }

  std::string res = '<' + uri.substr(pos + 1);
  return res;
}

// ________________________________________________________________
template <class Archive>
void EntityFinder::serialize(Archive& ar, const unsigned int version) {
  (void)version;
  ar& _subjectVecs;
  ar& _propertyVecs;
}
