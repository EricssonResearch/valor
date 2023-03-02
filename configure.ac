AC_INIT([valor],[1.0],[gabor.a.nemeth@ericsson.com])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects -Wno-portability])
AM_SILENT_RULES([yes])

: ${CXXFLAGS=""}
AC_PROG_CXX
AC_PROG_RANLIB

VALOR_CPPFLAGS=
VALOR_CXXFLAGS="-std=c++20 -g -Wall -Wextra -Wswitch-enum"

AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug], [compile with debug options])])
AS_IF([test "$enable_debug" = yes],
      [VALOR_CXXFLAGS="$VALOR_CXXFLAGS -O0 -ggdb3"],
      [VALOR_CPPFLAGS="$VALOR_CPPFLAGS -DNDEBUG"
       VALOR_CXXFLAGS="$VALOR_CXXFLAGS -Ofast"])

AC_CHECK_PROGS([LC], [llvm-config], [:])
AS_IF([test "$LC" = :], [AC_MSG_ERROR([Cannot find llvm-config; `apt install llvm`?])])
CLANG_LIBS=
for i in Tooling Frontend Driver Serialization Sema Analysis AST ASTMatchers Parse Edit Lex Basic Rewrite
do CLANG_LIBS="$CLANG_LIBS -lclang$i";
done

PKG_CHECK_MODULES([GT], [gtest_main])

AC_LANG_PUSH([C++])
save="$CPPFLAGS"
save2="$CXXFLAGS"
CPPFLAGS="-I${srcdir}"
CXXFLAGS="-std=c++20"
AC_CHECK_HEADER([ufser/ufser.h],, [AC_MSG_ERROR([cannot find ufser/ufser.h; did you do `git submodule checkout`?])])
CPPFLAGS="$save"
CXXFLAGS="$save2"
AC_LANG_POP

AC_SUBST([LLVM_CPP], [`$LC --cppflags`])
AC_SUBST([LLVM_CXX], ["`$LC --cxxflags` -std=c++20"]) dnl override
AC_SUBST([LLVM_LD], [`$LC --ldflags`])
AC_SUBST([LLVM_LIBS], ["$CLANG_LIBS `$LC --libs --system-libs`"])

AC_SUBST([GTEST_LIBS], [$GT_LIBS])

AC_SUBST([AM_CPPFLAGS], [$VALOR_CPPFLAGS])
AC_SUBST([AM_CXXFLAGS], [$VALOR_CXXFLAGS])

AC_SUBST([ARFLAGS], [cr])

AC_CONFIG_FILES([
    Makefile
    balor/Makefile
    tests/Makefile
])
AC_OUTPUT
