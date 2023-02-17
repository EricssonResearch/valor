ACLOCAL_AMFLAGS = -Im4
SUBDIRS = . tests #vapor

bin_PROGRAMS = valet #valor --> remove cruft from *FLAGS
noinst_LIBRARIES = libvalet.a

AM_DEFAULT_SOURCE_EXT = .cc

# valor_SOURCES = valor.cc ufser/ufser.cpp

AM_CPPFLAGS += $(LLVM_CPP)
AM_CXXFLAGS += $(LLVM_CXX)

valet_LDFLAGS = $(AM_LDFLAGS) $(LLVM_LD)
valet_LDADD = libvalet.a $(LLVM_LIBS)

libvalet_a_SOURCES = valet_lib.cc

# check: $(bin_PROGRAMS)
# 	valor s | valor r
# 	valet $(srcdir)/min.cc --extra-arg=-std=c++20 --extra-arg=-I$(srcdir) --
