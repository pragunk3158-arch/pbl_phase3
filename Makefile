# Build the project on Linux or Mac:  make
# Run it:                             make run
# Run the tests:                      make test
# Open the web dashboard:             make web

CXX = g++
FLAGS = -std=c++17 -Wall -Wextra -Iinclude
SOURCES = src/Bin.cpp src/Vehicle.cpp src/Graph.cpp src/Route.cpp src/WasteManager.cpp src/main.cpp
PROGRAM = SmartWaste

all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	$(CXX) $(FLAGS) $(SOURCES) -o $(PROGRAM)

run: $(PROGRAM)
	./$(PROGRAM)

demo: $(PROGRAM)
	./$(PROGRAM) --demo

test: $(PROGRAM)
	./$(PROGRAM) --test

web: $(PROGRAM)
	python3 server.py

clean:
	rm -f $(PROGRAM)
