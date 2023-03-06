check_PROGRAMS = cmp_sw
TESTS = once many
LOG_COMPILER = .
AM_DEFAULT_SOURCE_EXT = .cc
LDADD = -lboost_fiber -lboost_context
once:
	echo 'for i in f c C;do ./cmp_sw $$i 1;done' >$@
many:
	echo -e 'time ./cmp_sw f 10"000"000;time ./cmp_sw C 80"000"000' >$@
