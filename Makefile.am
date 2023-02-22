ACLOCAL_AMFLAGS = -Im4
SUBDIRS = . tests #vapor

bin_PROGRAMS = valet valor
noinst_LIBRARIES = libvalet.a

AM_DEFAULT_SOURCE_EXT = .cc

valet_CPPFLAGS = $(AM_CPPFLAGS) $(LLVM_CPP)
# valet_CXXFLAGS = $(AM_CXXFLAGS) $(LLVM_CXX)
valet_LDFLAGS = $(AM_LDFLAGS) $(LLVM_LD)
valet_LDADD = libvalet.a $(LLVM_LIBS)

libvalet_a_SOURCES = valet_lib.cc
libvalet_a_CPPFLAGS = $(AM_CPPFLAGS) $(LLVM_CPP)

valor_SOURCES = valor_valet.cc ufser/ufser.cpp

BUILT_SOURCES = valor_valet.cc

%_valet.cc: %.cc valet
	$(AM_V_GEN)xVALET_LOGLEVEL=debug ./valet -I$(srcdir) $<

check: valor
	./valor s | ./valor r
