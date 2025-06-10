CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LDFLAGS = -lstdc++fs

all: file_send file_recieve

file_send: file_send.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

file_recieve: file_recieve.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f file_send file_recieve

.PHONY: all clean 