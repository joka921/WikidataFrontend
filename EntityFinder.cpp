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
    auto* wdVec = &wdNameVec;
    auto* dVec = &descVec;
    auto* nVec = &nameVec;
    auto* aVec = &aliasVec;
    auto* sVec = &numSitelinkVec;
    if (WikidataEntity::IsPropertyName(entity.name)) {
      wdVec = &wdNameVecPred;
      dVec = &descVecPred;
      nVec = &nameVecPred;
      aVec = &aliasVecPred;
      sVec = &numSitelinkVecPred;
    }

    // push internal name (Q42, P31 etc.) and number of sitelinks
    wdVec->push_back(entity.name);
    sVec->push_back(entity.numSitelinks);

    if (entity.aliases.size() > 0) {
      // the first alias is the actual name of the entity
      nVec->push_back(entity.aliases[0]);
    } else {
      nVec->push_back("No readable name");
    }

    // push all aliases of the entity and the index of the current entity
    for ( auto& el : entity.aliases) {
      std::transform(el.begin(), el.end(), el.begin(), ::tolower);
      aVec->push_back(std::make_pair(el, wdVec->size() -1));
    }

    // get the description
    std::string tempDesc;
    if (std::getline(fileDesc, tempDesc)) {
      dVec->push_back(std::move(tempDesc));
    } else {
      dVec->push_back("no description");
    }
  }

  // sort the aliases alphabetically so we can perform binary search on them
  auto sortPred = [](const std::pair<std::string, unsigned>&  p1,
                     const std::pair<std::string, unsigned>&  p2) {
     return p1.first < p2.first;};
  std::sort(aliasVec.begin(), aliasVec.end(), sortPred);
  std::sort(aliasVecPred.begin(), aliasVecPred.end(), sortPred);

  // Set up the "translation indices" for the Q... entities

  // get highest id
  size_t maxIdxEntity = 0;
  for (const auto& el : wdNameVec) {
    size_t idx = getIdxFromWdName(el).first;
    maxIdxEntity = std::max(maxIdxEntity, idx);
  }

  // use an array with empty value to guarantee O(1) access
  EntityToIdxVec.resize(maxIdxEntity + 1);
  std::fill(EntityToIdxVec.begin(), EntityToIdxVec.end(), -1);

  size_t num = 0;
  for (const auto& el : wdNameVec) {
    size_t idx = getIdxFromWdName(el).first;
    EntityToIdxVec.at(idx) = num;
    num += 1;
  }

  // Set the same array up for the properties
  maxIdxEntity = 0;
  for (const auto& el : wdNameVecPred) {
    size_t idx = getIdxFromWdName(el).first;
    maxIdxEntity = std::max(maxIdxEntity, idx);
  }

  PropertyToIdxVec.resize(maxIdxEntity + 1);
  std::fill(PropertyToIdxVec.begin(), PropertyToIdxVec.end(), -1);

  num = 0;
  for (const auto& el : wdNameVecPred) {
    size_t idx = getIdxFromWdName(el).first;
    PropertyToIdxVec.at(idx) = num;
    num += 1;
  }

  // these are read only
  EntityToIdxVec.shrink_to_fit();
  PropertyToIdxVec.shrink_to_fit();
  aliasVec.shrink_to_fit();
  aliasVecPred.shrink_to_fit();
  descVec.shrink_to_fit();
  descVecPred.shrink_to_fit();
  nameVec.shrink_to_fit();
  nameVecPred.shrink_to_fit();
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
  auto* wdVec = &wdNameVec;
  auto* vec = &aliasVec;
  auto* dVec = &descVec;
  auto* nVec = &nameVec;
  auto* sVec = &numSitelinkVec;
  auto type = EntityType::Subject;
  if (mode == SearchMode::Properties) {
    vec = &aliasVecPred;
    dVec = &descVecPred;
    wdVec = &wdNameVecPred;
    nVec = &nameVecPred;
    sVec = &numSitelinkVecPred;
    type = EntityType::Property;
  }

  // Perfrom the actual search
  EntitySearchResult ret;

  // first find matches for the prefixes
  // lower bound
  auto res = std::lower_bound(vec->begin(), vec->end(), prefix, boundPred);
  if (res == vec->end() ||
      !std::equal(prefix.begin(), prefix.end(), (*res).first.begin())) {
    ret.totalResults = 0;
    return ret;
   }

   auto upper = std::upper_bound(res, vec->end(), prefix, upperBoundPred);
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

   // format is <numberOfSitelinks, Idx>
   // make sorting faster
   std::vector<std::pair<size_t,size_t>> onlyIdxVec;
   std::vector<std::pair<size_t,size_t>> onlyIdxVecExact;
   onlyIdxVec.reserve(upper - upperExact);
   onlyIdxVecExact.reserve(upperExact - res);

   // setup sitelink/idx vector for exact matches
   while (res != upperExact) {
     auto idx = (*res).second;
     onlyIdxVecExact.push_back(std::make_pair((*sVec)[idx], idx));
     res++;
   }
   // .. and for prefix matches
   while (res != upper) {
     auto idx = (*res).second;
     onlyIdxVec.push_back(std::make_pair((*sVec)[idx], idx));
     res++;
   }

   // predicate for sorting according to the sitelink score
   auto sortPred = [](const std::pair<size_t, size_t>& w1, const std::pair<size_t, size_t>& w2)
                      { return w1.first > w2.first;};
   // predicate for the elimination of equal
   auto equalPred = [](const std::pair<size_t, size_t>& w1, const std::pair<size_t, size_t>& w2)
                      { return w1.second == w2.second;};
   // reversed order, because we want high number of sitelinks first
   std::sort(onlyIdxVec.begin(), onlyIdxVec.end(), sortPred);
   std::sort(onlyIdxVecExact.begin(), onlyIdxVecExact.end(), sortPred);
   auto upperUnique = onlyIdxVec.end(); //std::unique(onlyIdxVec.begin(), onlyIdxVec.end(), equalPred);
   auto upperUniqueExact = onlyIdxVecExact.end(); //std::unique(onlyIdxVecExact.begin(), onlyIdxVecExact.end(), equalPred);
   auto uniqueSize = upperUnique - onlyIdxVec.begin();
   auto uniqueSizeExact = upperUniqueExact - onlyIdxVecExact.begin();

   for (int i = 0; i < 40 && i < uniqueSizeExact; ++i) {
     auto idx = onlyIdxVecExact[i].second;
     auto numLinks = onlyIdxVecExact[i].first;
     ret.entities.push_back(WikidataEntityShort((*wdVec)[idx], (*nVec)[idx], (*dVec)[idx], numLinks));
   }
   for (int i = ret.entities.size(); i < 40 && i < uniqueSize; ++i) {
     auto idx = onlyIdxVec[i].second;
     auto numLinks = onlyIdxVec[i].first;
     ret.entities.push_back(WikidataEntityShort((*wdVec)[idx], (*nVec)[idx], (*dVec)[idx], numLinks));
   }

   auto translateTime = std::chrono::high_resolution_clock::now();
   std::cout << "took" << std::chrono::duration_cast<std::chrono::milliseconds>(findTime - startTime).count() << " ms to find" << std::endl;
   std::cout << "took" << std::chrono::duration_cast<std::chrono::milliseconds>(translateTime - findTime).count() << " ms to translate to readable" << std::endl;
   return ret;
 }

// ________________________________________________________________________
std::pair<size_t, EntityType> EntityFinder::getIdxFromWdName(const std::string& wdName) {
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
    if (pos == std::string::npos || pos == wdName.size() - 1 || (wdName[pos + 1] != 'Q' && wdName[pos + 1] != 'P')) {
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
WikidataEntityShort EntityFinder::wdNamesToEntities(const std::string& el) const {
    auto p = getIdxFromWdName(el);
    auto& idx = p.first;
    auto* vec = &EntityToIdxVec;
    auto* nVec = &nameVec;
    auto* dVec = &descVec;
    auto* sVec = &numSitelinkVec;
    if (p.second == EntityType::Property) {
      vec = &PropertyToIdxVec;
      nVec = &nameVecPred;
      dVec = &descVecPred;
      sVec = &numSitelinkVecPred;
    }
    // default values which make sense for everything that is NOT a
    // wikidata-entity
    std::string wdName = "";
    
    std::string name = el;
    std::string desc = "";
    unsigned int numSitelinks = 0;
    if (idx < vec->size()) {
      // convert from the "wikidata-name-idx" to the internal (unique) index
      auto wdIdx = idx;
      idx = (*vec)[idx];
      if (idx <= nVec->size()) {
        // if there is an entity matching, then also include name and description
        name = (*nVec)[idx];
        desc = (*dVec)[idx];
	numSitelinks = (*sVec)[idx];
	
	// compose wdName in the canonical "internal" form (<Q42> or <P31>)
	wdName = "<" + entityTypeToString(p.second) + std::to_string(wdIdx) + ">";
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

// ________________________________________________________________
template<class Archive>
void EntityFinder::serialize(Archive& ar, const unsigned int version){
  ar & wdNameVec;
  ar & wdNameVecPred;
  ar & EntityToIdxVec;
  ar & PropertyToIdxVec;
  ar & descriptionFilename;
  ar & descVec;
  ar & descVecPred;
  ar & nameVec;
  ar & nameVecPred;
  ar & aliasVec;
  ar & aliasVecPred;
  ar & numSitelinkVec;
  ar & numSitelinkVecPred;
}
