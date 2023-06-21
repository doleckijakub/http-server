CXX ?= g++

CPPFLAGS := -Isrc
CXXFLAGS := -Wall -Wextra -Wswitch-enum -pedantic -O2
LDFLAGS :=

SRCDIR ?= src

all: build/exception.o build/ip.o build/url.o build/response.o build/request.o build/host.o build/server.o | build

build:
	mkdir -p build

build/%.o: $(SRCDIR)/%.cpp | build
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

build/response.o: $(SRCDIR)/response.cpp $(SRCDIR)/content_type.hpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

build/request.o: $(SRCDIR)/request.cpp $(SRCDIR)/method.hpp build/url.o build/response.o
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

build/host.o: $(SRCDIR)/host.cpp build/ip.o
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

build/server.o: $(SRCDIR)/server.cpp $(SRCDIR)/log.hpp build/request.o build/host.o
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

#

example: example.cpp all
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $< build/*.o $(LDFLAGS)

.PHONY: test
test: example
	./example 8080

.PHONY: clean
clean:
	@FILES=$$(git clean -ndX); \
	if [ -z "$$FILES" ]; then \
		echo "No deletable files, skipping"; \
	else \
		echo "These files will be deleted:"; \
		echo "$$FILES"; \
		read -p "Do you want to delete them? [N/y] " yn; \
		if [ "$$yn" = "y" ] || [ "$$yn" = "Y" ]; then \
			git clean -fdX; \
		else \
			echo "Deletion cancelled."; \
		fi \
	fi