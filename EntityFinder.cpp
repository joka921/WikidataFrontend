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

    wdVec->push_back(entity.name);
    sVec->push_back(entity.numSitelinks);
    if (entity.aliases.size() > 0) {
      nVec->push_back(entity.aliases[0]);
    } else {
      nVec->push_back("No readable name");
    }

    for ( auto& el : entity.aliases) {
      std::transform(el.begin(), el.end(), el.begin(), ::tolower);
      // TODO: substring memory consumption test
      aVec->push_back(std::make_pair(el, wdVec->size() -1));
    }

    // for each line in alias file there must exist exactly one line in
    // description file. We don't want the description but only the offsets
    // TODO: is there a way to do this without actually reading the file?
    //descVec->push_back(fileDesc.tellg());
    std::string tempDesc;
    std::getline(fileDesc, tempDesc);
    dVec->push_back(tempDesc);
  }
  auto sortPred = [](const std::pair<std::string, unsigned>&  p1,
                     const std::pair<std::string, unsigned>&  p2) {
     return p1.first < p2.first;};
  std::cout << aliasVec.size() << "entities" << std::endl;
  std::cout << aliasVecPred.size() << "Predicates" << std::endl;
  std::sort(aliasVec.begin(), aliasVec.end(), sortPred);
  std::sort(aliasVecPred.begin(), aliasVecPred.end(), sortPred);
  

  // Set up the "translation indices" for the sub/objects
  size_t maxIdxEntity = 0;
  for (const auto& el : wdNameVec) {
    size_t idx = getIdxFromWdName(el).first;
    maxIdxEntity = std::max(maxIdxEntity, idx);
  }
  EntityToIdxVec.resize(maxIdxEntity + 1);
  std::fill(EntityToIdxVec.begin(), EntityToIdxVec.end(), -1);

  size_t num = 0;
  for (const auto& el : wdNameVec) {
    size_t idx = getIdxFromWdName(el).first;
    EntityToIdxVec.at(idx) = num;
    num += 1;
  }

  // same for the properties
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
  std::string prefix = prefixA;
  std::ifstream descFile(descriptionFilename);
  std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);
  auto boundPred = [](const std::pair<std::string, unsigned>&  p1,
                     const std::string& p2) { return p1.first < p2;};
  auto upperBoundPred = [](const std::string& p1, const std::pair<std::string, unsigned>&  p2) 
  { return p1 < p2.first.substr(0, std::min(p2.first.size(), p1.size()));};
  auto upperBoundPredExact = [](const std::string& p1, const std::pair<std::string, unsigned>&  p2) 
  { return p1 <= p2.first;};

  //TODO: some meaningful mixing of subjects and properties for searchmode "all"
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
   EntitySearchResult ret;
   auto res = std::lower_bound(vec->begin(), vec->end(), prefix, boundPred);
   if (res == vec->end() || !std::equal(prefix.begin(), prefix.end(), (*res).first.begin())) {
     // no real results found
     ret.totalResults = 0;
     return ret;
   }

   auto upper = std::upper_bound(res, vec->end(), prefix, upperBoundPred);
   auto upperExact = std::lower_bound(res, upper, prefix + ' ', boundPred);
   auto findTime = std::chrono::high_resolution_clock::now();
   auto numPrefixMatches = ret.totalResults = upper - res;
   auto numExactMatches = upperExact - res;
   std::cout << prefix << std::endl;
   std::cout << "found " << upper - res << std::endl;
   //TODO parametrize this and experiment
   size_t maxRelevant = 100000;
   

   if (numPrefixMatches > maxRelevant) {
     upper = res + maxRelevant;
     if (numExactMatches > maxRelevant) {
       upperExact = res + maxRelevant;
     }
   }

   std::vector<std::pair<size_t,size_t>> onlyIdxVec;
   std::vector<std::pair<size_t,size_t>> onlyIdxVecExact;
   onlyIdxVec.reserve(upper - upperExact);
   onlyIdxVecExact.reserve(upperExact - res);
   while (res != upperExact) {
     auto idx = (*res).second;
     onlyIdxVecExact.push_back(std::make_pair((*sVec)[idx], idx));
     res++;
   }
   while (res != upper) {
     auto idx = (*res).second;
     onlyIdxVec.push_back(std::make_pair((*sVec)[idx], idx));
     //ret.entities.push_back(WikidataEntityShort((*wdVec)[idx], (*nVec)[idx], (*dVec)[idx]));
     //std::cout << ret.size() << std::endl;
     res++;
   }
   // TODO: this way we do not eliminate hits, but we also do not eliminate
   // all duplicates
   auto sortPred = [](const std::pair<size_t, size_t>& w1, const std::pair<size_t, size_t>& w2)
                      { return w1.first > w2.first;};
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

 /*
// ________________________________________________________________________
std::string EntityFinder::readSingleDescription(std::ifstream* descFile, size_t internalIdx, EntityType type) const {
  auto* descVec = type==EntityType::Subject ? &descOffsetVec : &descOffsetVecPred;
   if (descFile->is_open()) {
     descFile->seekg((*descVec)[internalIdx]);
   }
   std::string desc;
   std::getline(*descFile, desc);
   return desc;
}
*/

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
    std::cout << "Reading from preprocessed file " << filenamePreprocessed
              << " if this is not desired (e.g. after updating the input data) "
                 "please rename or remove this file\n";
    return ReadFromFile(filenamePreprocessed);
  } else {
    EntityFinder finder;
    finder.InitializeFromTextFile(filename);
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


