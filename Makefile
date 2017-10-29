# contrib/btree_gist/Makefile

MODULE_big = pg_gist_hamming

OBJS =  btree_gist.o      \
		$(WIN32RES)

# 		btree_utils_var.o \

EXTENSION = pg_gist_hamming

DATA = \
	pg_gist_hamming--1.0.sql

	# pg_gist_hamming--1.2--1.3.sql           \
	# pg_gist_hamming--1.3--1.4.sql           \
	# pg_gist_hamming--1.4--1.5.sql
	# pg_gist_hamming--unpackaged--1.0.sql \
	# pg_gist_hamming--1.0--1.1.sql           \
	# pg_gist_hamming--1.1--1.2.sql           \


PGFILEDESC = "pg_gist_hamming - B-tree equivalent GiST operator classes"

REGRESS = init int8 int8_2 # not_equal

SHLIB_LINK += $(filter -lm, $(LIBS))

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_gist_hamming
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
