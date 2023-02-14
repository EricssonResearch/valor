ACLOCAL_AMFLAGS = -Im4
SUBDIRS = . #vapor tests

bin_PROGRAMS = valet valor

AM_DEFAULT_SOURCE_EXT = .cc

valor_SOURCES = valor.cc ufser/ufser.cpp

valet_CPPFLAGS = $(AM_CPPFLAGS) $(LLVM_CPP)
valet_CXXFLAGS = $(AM_CXXFLAGS) $(LLVM_CXX)
valet_LDFLAGS = $(AM_LDFLAGS) $(LLVM_LD)
valet_LDADD = $(LLVM_LIBS)

check: $(bin_PROGRAMS)
# 	valor s | valor r
	valet $(srcdir)/valor.cc --extra-arg=-std=c++20 --extra-arg=-I$(srcdir) --
