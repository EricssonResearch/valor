# https://gcc.gnu.org/onlinedocs/gccint/Plugins-building.html#Plugins-building
PLUGINDIR = $(shell $(CXX) -print-file-name=plugin)
CPPFLAGS = -I$(PLUGINDIR)/include
CXXFLAGS = -fno-rtti -Wall -fpic

vapor.so:

%.so: %.o
	$(CXX) -shared -o $@ $<

# vapor.o : vapor.cc
#     $(CXX) $(CXXFLAGS) -c -o $@ $<

# clean:
# 	rm -f vapor.*o

check: vapor.so
	$(CXX) -fplugin=$< -c -std=c++20 ../toy.cc -o /dev/null

.PHONY: check