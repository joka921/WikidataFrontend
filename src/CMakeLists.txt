
add_library(SearchServer
            EntitySearchResult.h  HTTPClient.h
	    QLeverResult.h ServerUtils.h
            EntityFinder.cpp  HTTPClientBoost.h     
	    QLeverCommunicator.cpp  SearchServer.cpp  
	    SearchServerMainTest.cpp  WikidataEntity.cpp
            EntityFinder.h HTTPClient.cpp
	    QLeverCommunicator.h SearchServer.h  
	    ServerUtils.cpp WikidataEntity.h
	    )
#target_link_libraries(index parser ${STXXL_LIBRARIES})

