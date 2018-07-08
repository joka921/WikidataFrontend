CXX = g++ -O3  -Wall -std=c++14 -L/home/kalmbacj/lib -L curl/curlbuild/lib/
MAIN_BINARIES = $(basename $(wildcard *Main.cpp))
TEST_BINARIES = $(basename $(wildcard *Test.cpp))
HEADER = $(wildcard *.h)
OBJECTS = $(addsuffix .o, $(basename $(filter-out %Main.cpp %Test.cpp, $(wildcard *.cpp))))
CPPLINT_PATH = ./cpplint.py
CPPLINT_FILTERS = -runtime/references,-build/header_guard,-build/include

PORT = 9998
FILE = /nfs/raid5/kalmbacj/SplitParsing/withPreferredComplete

.PRECIOUS: %.o

all: compile test checkstyle

compile: $(MAIN_BINARIES) $(TEST_BINARIES)

test: $(TEST_BINARIES)
	for T in $(TEST_BINARIES); do ./$$T; done

checkstyle:
	@# Filter header_guards and include check, doesn't work well for
	@# projects roots that are not a svn or git root.
	@# Allow non-const references.
	python $(CPPLINT_PATH) --filter='$(CPPLINT_FILTERS)' *.h *.cpp

clean:
	rm -f *.o
	rm -f $(MAIN_BINARIES)
	rm -f $(TEST_BINARIES)
	rm -f *.class
	rm -f *Test.TMP.*
	rm -f core

%Main: %Main.o $(OBJECTS)
	$(CXX) -o $@ $^ -lboost_system -lboost_serialization -lcurl

%Test: %Test.o $(OBJECTS)
	$(CXX) -o $@ $^ -lgtest -lgtest_main -lpthread -lboost_system -lboost_serialization -lcurl

%.o: %.cpp $(HEADER)
	$(CXX) -c $<

start: SearchServerMain
	./SearchServerMain $(FILE) $(PORT) 
