
add_library(SearchServer
        EntityFinderHelpers.h
		ServerUtils.h
            EntityFinder.cpp     
	    QLeverCommunicator.cpp  SearchServer.cpp  
	    SearchServerMainTest.cpp
        EntityFinder.h
	    QLeverCommunicator.h SearchServer.h  
	    ServerUtils.cpp WikidataEntityParse.h
	    )
#target_link_libraries(index parser ${STXXL_LIBRARIES})

