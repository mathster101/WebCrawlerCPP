# Compiler and build flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O3

# External libraries to link (curl for HTTP, gumbo for HTML parsing)
LDLIBS = -lcurl -lgumbo

# Output executable name
TARGET = webCrawler

# Source files and their compiled object files
SRCS = main.cpp crawlers.cpp utils.cpp
OBJS = $(SRCS:.cpp=.o)

# Default target: build the executable
all: $(TARGET)

# Link all object files into the final executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDLIBS)

# Compile each .cpp into a .o file
# Also depend on the headers so recompilation happens when headers change
%.o: %.cpp crawlers.h utils.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Remove generated object files and executable
clean:
	rm -f $(OBJS) $(TARGET)

# These targets don't produce actual files, so mark them as phony
.PHONY: all clean
