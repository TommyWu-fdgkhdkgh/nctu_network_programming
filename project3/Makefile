CXX=clang++
CXXFLAGS=-std=c++14 -Wall -pedantic -pthread -lboost_system -I /usr/local/include -L /usr/local/lib -pthread -lboost_filesystem -lboost_system 
CXX_INCLUDE_DIRS=-I /usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=-L /usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))

server : 
	$(CXX) echo_server.cpp  -o http_server $(CXXFLAGS)

console :
	$(CXX) console.cpp -o console.cgi $(CXXFLAGS)

clean:
	rm -f  echo_server
