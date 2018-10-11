# WikidataFrontend + EntityFinder

This repository provides the following functionality:

* Efficient Search of Wikidata Entities using prefixes of their English names and aliases (the *EntityFinder*)
* A simple Drag-And-Drop user interface based on the *EntityFinder* that can be used to create and execute simple SPARQL queries on Wikidata using the QLever SPARQL engine.

## Getting Started
#### 0. Requirements
When running inside a docker container:
* docker 18.05 or newer (needs multi-stage builds without leaking files (for End-to-End Tests))

When running as a stand-alone software
* A Linux system that is able to build C++14 software
* CMake
* Development version of the boost libraries (at least `libboost-system-dev` and `libboost-serialization-dev`)
* Development version of `libcurl`

(Optional for the Execution of SPARQL queries)
* A running instance of [QLever](https://github.com/ad-freiburg/QLever "QLever Github Repository") using the [Wikidata](https://www.wikidata.org) knowledge base
 (the necessary dumps of Wikidata can be found [here](https://dumps.wikimedia.org/wikidatawiki/entities/))
 
 
#### 1. Creating Input Data
The *EntityFinder* needs the following files (`<prefix>` can be chosen by the user):
* A text file `<prefix>.entities` that contains one line for each entity using the folowing format:
```
 <WikidataId>    <NumberOfSitelinks> <Alias1>    <Alias2>...
 ```
* All entries must be separated by tabs
* `<WikidataId>` is the Q... or P... Id in Wikidata surrounded by angles, so e.g. `<Q42>` for "Douglas Adams" or `<P31>` for the property "instance-of"
* `<NumberOfSitelinks>` is an integer score that is used to rank the search results. We originally use the number of sitelinks of a Wikidata entity, but this can be an arbitrary score.
* `<AliasX>` is a human-readable name of the entity. The first alias of an entity will be used as the entitie's *name* inside the EntityFinder whereas the other aliases are only used as aliases during the search.

* We also need a text file `<prefix>.desc` where the i-th line contains a description string of the item on the i-th line in the `<prefix>.entities` file.

* An short example for this format can be found in the `examples/` subfolder of the repository.

* We also provided a script that can read the json dump of Wikidata and create those files automatically. To use it, download the compressed json dump `latest-all.json.bz2`
  from [here](https://dumps.wikimedia.org/wikidatawiki/entities/). And use the `extractEntities.py` in the main folder of the repository:
  
  ``` python3 extractEntities.py latest-all.json.bz2 <prefix>```


#### Running the Backend Using Docker
(all steps must be performed inside the repositories main directory)
* Build the Docker Container
```
docker build -t wikidata-frontend-<name> .
```
* Move your input files `<prefix>.entities` and `<prefix>.desc` to the `input/` subfolder.
* Make sure that the input folder is accessible from within the container
```
chmod -R a+rw ./input
```
* Run the Container
```
docker run -d -p 7001:7001 -e "INPUT_PREFIX=<prefix>" -v "$(pwd)/input:/input" -e "QLEVER_ADDRESS=<ip-of-qlever-server> -e "QLEVER_PORT=<port-of-qlever-server"--name wikidata-frontend-<name> wikidata-frontend-<name>

```
* Note: if the address and port of the QLever server are omitted the Entity finder will still work, but you will not be able to execute SPARQL queries
* Open a webbrowser and go to
`localhost:7001` (or whichever machine you are running the container on) to access the frontend.

#### Running the Backend Using Docker
(all steps must be performed inside the repositories main directory)
* Build the software
```
 mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)
```

* Run the software (we need files `<path-to-prefix>.entities` and `<path-to-prefix>.desc` as described above)
```
./WikidataFrontendMain <path-to-prefix> <port> <ip-of-qlever> <port-of-qlever>```
```
The two QLever-related arguments must either both be present or both omitted.
If no QLever server can be reached at this adress you can not perform SPARQL queries.

* Open a webbrowser and go to
`localhost:7001` (or whichever machine you are running the WikidataFrontend on) to access the frontend.
