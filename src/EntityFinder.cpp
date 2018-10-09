#include "EntityFinder.h"

#include <fstream>
#include <ios>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <chrono>

// ______________________________________________________________________
void EntityFinder::InitializeFromTextFile(const std::string& filename) {
  std::ifstream file(filename);
  descriptionFilename = filename + ".desc";
  std::ifstream fileDesc(descriptionFilename);
  if (!(file && fileDesc)) {
    std::cerr
        << "ERROR: either the name file " << filename
        << " or the description filename " << descriptionFilename
        << "could not been opened. Search engine will contain no entities.\n";
    return;
  }
  std::string line;

  while (std::getline(file, line)) {
    auto entity = WikidataEntity(line);
    // Determine whether this is a property (P... or a entity (Q...)
    // and determine the correct data structures to store the data
    auto &v = WikidataEntity::isPropertyName(entity._wdName) ? _propertyVecs
                                                          : _entityVecs;
    // push internal name (Q42, P31 etc.) and number of sitelinks
    // get the description
    auto entityShort = WikidataEntityShort(entity);
    std::string tempDesc;
    if (!std::getline(fileDesc, tempDesc)) {
      tempDesc = "no description";
    }
    entityShort._description = tempDesc;
    v._entities.push_back(std::move(entityShort));

    // push all aliases of the entity and the index of the current entity
    for ( auto& el : entity._aliases) {
      std::transform(el.begin(), el.end(), el.begin(), ::tolower);
      v._aliases.push_back(std::make_pair(el, v._entities.size() - 1));
    }
  }

  // sort the aliases alphabetically so we can perform binary search on them
  auto sortPred = [](const std::pair<std::string, unsigned>&  p1,
                     const std::pair<std::string, unsigned>&  p2) {
     return p1.first < p2.first;};
  std::sort(_entityVecs._aliases.begin(), _entityVecs._aliases.end(), sortPred);
  std::sort(_propertyVecs._aliases.begin(), _propertyVecs._aliases.end(),
            sortPred);

  // Set up the "translation indices" for the Q... entities
  // get highest id

  std::array<EntityVectors *, 2> vecs = {&_entityVecs, &_propertyVecs};
  for (auto vPtr : vecs) {
    auto &v = *vPtr;
    size_t maxIdxEntity = 0;
    for (const auto &el : v._entities) {
      const auto& wd = el._wdName;
      size_t idx = getIdxFromWdName(wd).first;
      maxIdxEntity = std::max(maxIdxEntity, idx);
    }

    // use an array with empty value to guarantee O(1) access
    v._entityToIdx.resize(maxIdxEntity + 1);
    std::fill(v._entityToIdx.begin(), v._entityToIdx.end(), -1);

    size_t num = 0;
    for (const auto &el : v._entities) {
      const auto& wd = el._wdName;
      size_t idx = getIdxFromWdName(wd).first;
      v._entityToIdx.at(idx) = num;
      num += 1;
    }
    v.shrink_to_fit();
  }
}

// __________________________________________________________________
EntitySearchResult EntityFinder::findEntitiesByPrefix( const std::string& prefixA, SearchMode mode) 
{
  auto startTime = std::chrono::high_resolution_clock::now();
  // mutable version of argument
  std::string prefix = prefixA;
  std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);

  // lambda functions for upper and lower bound for prefix and exact matches
  auto boundPred = [](const std::pair<std::string, unsigned>&  p1,
                     const std::string& p2) { return p1.first < p2;};
  auto upperBoundPred = [](const std::string& p1, const std::pair<std::string, unsigned>&  p2) 
  { return p1 < p2.first.substr(0, std::min(p2.first.size(), p1.size()));};
  auto upperBoundPredExact = [](const std::string& p1, const std::pair<std::string, unsigned>&  p2) 
  { return p1 <= p2.first;};

  // choose the correct set of internal data structures depending on
  // the search type (Properties or other Entities)
  const auto &v = mode == SearchMode::Properties ? _propertyVecs : _entityVecs;

  // Perform the actual search
  EntitySearchResult ret;

  // first find matches for the prefixes
  // lower bound
  auto res =
      std::lower_bound(v._aliases.begin(), v._aliases.end(), prefix, boundPred);
  if (res == v._aliases.end() ||
      !std::equal(prefix.begin(), prefix.end(), (*res).first.begin())) {
    ret.totalResults = 0;
    return ret;
   }

   auto upper = std::upper_bound(res, v._aliases.end(), prefix, upperBoundPred);
   // upper bound for the exact matches
   auto upperExact = std::lower_bound(res, upper, prefix + ' ', boundPred);
   auto findTime = std::chrono::high_resolution_clock::now();
   auto numPrefixMatches = ret.totalResults = upper - res;
   auto numExactMatches = upperExact - res;
   std::cout << prefix << std::endl;

   // If we get more results than this we do not calculate the exact ranking but
   // truncate the result alphabetically
   const size_t maxRelevant = 100000;

   if (numPrefixMatches > maxRelevant) {
     upper = res + maxRelevant;
     if (numExactMatches > maxRelevant) {
       upperExact = res + maxRelevant;
     }
   }

   auto sortedIndices = rankResults(res, upperExact, upper, v);

   ret = convertIdxVecsToSearchResult(sortedIndices.first, sortedIndices.second,
                                      v);
   ret.totalResults = numPrefixMatches;

   auto translateTime = std::chrono::high_resolution_clock::now();
   std::cout << "took" << std::chrono::duration_cast<std::chrono::milliseconds>(findTime - startTime).count() << " ms to find" << std::endl;
   std::cout << "took" << std::chrono::duration_cast<std::chrono::milliseconds>(translateTime - findTime).count() << " ms to translate to readable" << std::endl;
   return ret;
}

// ______________________________________________________________________
EntitySearchResult
EntityFinder::convertIdxVecsToSearchResult(const IdxVec &exactIndices,
                                           const IdxVec &prefixIndices,
                                           const EntityVectors &v) {
  EntitySearchResult ret;
  ret.entities.reserve(RESULTS_TO_SEND);
  // first the matches
  for (int i = 0; i < RESULTS_TO_SEND && i < exactIndices.size(); ++i) {
    auto idx = exactIndices[i].second;
    auto numLinks = exactIndices[i].first;
    ret.entities.push_back(v._entities[idx]);
  }
  for (int i = ret.entities.size(); i < 40 && i < prefixIndices.size(); ++i) {
    auto idx = prefixIndices[i].second;
    auto numLinks = prefixIndices[i].first;
    ret.entities.push_back(v._entities[idx]);
  }
  return ret;
 }

 // _______________________________________________________________________
 std::pair<EntityFinder::IdxVec, EntityFinder::IdxVec>
 EntityFinder::rankResults(AliasIt lower, AliasIt upperExact,
                           AliasIt upperPrefixes, const EntityVectors &v) {
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
     onlyIdxVecExact.push_back(std::make_pair(ent[idx]._numSitelinks, idx));
     lower++;
   }
   // .. and for prefix matches
   while (lower != upperPrefixes) {
     auto idx = (*lower).second;
     onlyIdxVec.push_back(std::make_pair(ent[idx]._numSitelinks, idx));
     lower++;
   }

   // predicate for sorting according to the sitelink score
   auto sortPred = [](const std::pair<size_t, size_t> &w1,
                      const std::pair<size_t, size_t> &w2) {
     return w1.first != w2.first ? w1.first > w2.first : w1.second > w2.second;
   };
   // predicate for the elimination of duplicates
   auto equalPred = [](const std::pair<size_t, size_t> &w1,
                       const std::pair<size_t, size_t> &w2) {
     return w1.second == w2.second;
   };
   // reversed order, because we want high number of sitelinks first
   std::sort(onlyIdxVec.begin(), onlyIdxVec.end(), sortPred);
   std::sort(onlyIdxVecExact.begin(), onlyIdxVecExact.end(), sortPred);
   onlyIdxVec.erase(
       std::unique(onlyIdxVec.begin(), onlyIdxVec.end(), equalPred),
       onlyIdxVec.end());
   onlyIdxVecExact.erase(
       std::unique(onlyIdxVecExact.begin(), onlyIdxVecExact.end(), equalPred),
       onlyIdxVecExact.end());
   return std::make_pair(std::move(onlyIdxVecExact), std::move(onlyIdxVec));
 }

 // ________________________________________________________________________
 std::pair<size_t, EntityType>
 EntityFinder::getIdxFromWdName(const std::string &wdName) {
   // start with "<Q" or "<P", then number
   EntityType type = EntityType::Subject;
   if (wdName.size() < 3 || wdName[0] == '"') {
     // this is a string literal
     return std::make_pair(-1, type);
   }
   if (wdName[0] == '<') {
     // internal format
     if (wdName[1] == 'P') {
       type = EntityType::Property;
     } else if (wdName[1] == 'Q') {
       type = EntityType::Subject;
     } else {
       return std::make_pair(-1, type);
     }
     std::stringstream s(wdName.substr(2));
     size_t idx;
     s >> idx;
     return std::make_pair(idx, type);
   } else {
     auto pos = wdName.find(':');
     if (pos == std::string::npos || pos == wdName.size() - 1 ||
         (wdName[pos + 1] != 'Q' && wdName[pos + 1] != 'P')) {
       return std::make_pair(-1, type);
     }
     type = wdName[pos + 1] == 'Q' ? EntityType::Subject : EntityType::Property;
     std::stringstream s(wdName.substr(pos + 2));
     size_t idx;
     s >> idx;
     return std::make_pair(idx, type);
   }
}

// _______________________________________________________________________________
std::vector<std::vector<WikidataEntityShort>> EntityFinder::wdNamesToEntities(const std::vector<std::vector<string>>& wdNames) {
  std::vector<std::vector<WikidataEntityShort>> res;
  for (const auto& vec : wdNames) {
    res.push_back(wdNamesToEntities(vec));
  }
  return res;

}
// ___________________________________________________________________
std::vector<WikidataEntityShort> EntityFinder::wdNamesToEntities(const std::vector<string>& wdNames) {
  std::vector<WikidataEntityShort> ret;
  for (const auto& el : wdNames) {
    ret.push_back(wdNamesToEntities(el));
  }
  return ret;
}

// ________________________________________________________________________________
WikidataEntityShort EntityFinder::wdNamesToEntities(const std::string& in) const {
    auto el = ExtractWikidataIdFromUri(in);
    auto p = getIdxFromWdName(el);
    auto &idx = p.first;
    const auto &v =
        p.second == EntityType::Property ? _propertyVecs : _entityVecs;
    // default values which make sense for everything that is NOT a
    // wikidata-entity
    std::string wdName = "";
    
    std::string name = el;
    std::string desc = "";
    unsigned int numSitelinks = 0;
    if (idx < v._entityToIdx.size()) {
      idx = v._entityToIdx[idx];
      if (idx <= v._entities.size()) {
        return v._entities[idx];
      }
    }
    return WikidataEntityShort(wdName, name, desc, numSitelinks);
}

// ______________________________________________________________________________
void EntityFinder::WriteToFile(const std::string& filename) {

  std::cout << "Writing to file " << filename << std::endl;
  std::ofstream os(filename, std::ios::binary);
  boost::archive::binary_oarchive oar(os);
  oar << *this;
  std::cout << "Done." << std::endl;
}

// ______________________________________________________________________________
EntityFinder EntityFinder::ReadFromFile(const std::string& filename) {
  std::cout << "Reading from file " << filename << std::endl;
  std::ifstream os(filename, std::ios::binary);
  boost::archive::binary_iarchive oar(os);
  EntityFinder ent;
  oar >> ent;
  std::cout << "Done." << std::endl;
  return ent;
}

// _____________________________________________________________________________
EntityFinder EntityFinder::SetupFromFilename(const std::string &filename) {
  std::string filenamePreprocessed = filename + ".preprocessed.dat";
  std::cout << "Checking if preprocessed file exists\n";
  std::ifstream is(filenamePreprocessed, std::ios::binary);
  if (is) {
    is.close();
    std::cout
        << "Reading from preprocessed file " << filenamePreprocessed
        << ".\nIf this is not desired (e.g. after updating the input data),\n"
           "please rename or remove this file\n";
    return ReadFromFile(filenamePreprocessed);
  } else {
    std::cout << "no preprocessed file found, reading from original file "
              << filename << ".\n";
    EntityFinder finder;
    finder.InitializeFromTextFile(filename);
    finder.WriteToFile(filenamePreprocessed);
    return std::move(finder);
  }
}

std::string EntityFinder::ExtractWikidataIdFromUri(const string& uri) const {
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
  std::cout << res << '\n';
  return res;
}

// ________________________________________________________________
template<class Archive>
void EntityFinder::serialize(Archive& ar, const unsigned int version){
  ar &_entityVecs;
  ar &_propertyVecs;
}

