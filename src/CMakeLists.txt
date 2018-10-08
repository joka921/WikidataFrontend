
add_library(SearchServer
            EntitySearchResult.h 
	    QLeverResult.h ServerUtils.h
            EntityFinder.cpp     
	    QLeverCommunicator.cpp  SearchServer.cpp  
	    SearchServerMainTest.cpp  WikidataEntity.cpp
            EntityFinder.h
	    QLeverCommunicator.h SearchServer.h  
	    ServerUtils.cpp WikidataEntity.h
	    )
#target_link_libraries(index parser ${STXXL_LIBRARIES})

