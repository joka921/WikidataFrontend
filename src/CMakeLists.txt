
add_library(SearchServer
            EntitySearchResult.h 
		ServerUtils.h
            EntityFinder.cpp     
	    QLeverCommunicator.cpp  SearchServer.cpp  
	    SearchServerMainTest.cpp
        EntityFinder.h
	    QLeverCommunicator.h SearchServer.h  
	    ServerUtils.cpp WikidataEntity.h
	    )
#target_link_libraries(index parser ${STXXL_LIBRARIES})

