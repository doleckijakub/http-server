CXX ?= g++

default: build/example

CPPFLAGS :=
CXXFLAGS := -Wall -Wextra -Wswitch-enum -pedantic -O2
LDFLAGS :=

SRCDIR ?= src

build/example: $(SRCDIR)/example.cpp build/http.o
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o build/example $^ $(LDFLAGS)

build:
	mkdir -p build

build/%.o: $(SRCDIR)/%.cpp | build
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

.PHONY: test
test: build/example
	./build/example 8080

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