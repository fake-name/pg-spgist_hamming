

MODULE_big = vptree
OBJS = vptree.o

EXTENSION = vptree
DATA = vptree--1.0.sql

REGRESS = init test_1
# REGRESS = init int8 int8_2 # not_equal


PGFILEDESC = "vptree - Viewpoint Tree GiST Index operator"


ifdef USE_PGXS
	PG_CONFIG = pg_config
	PGXS := $(shell $(PG_CONFIG) --pgxs)
	include $(PGXS)
else
	subdir = contrib/pg_gist_hamming/vptree
	top_builddir = ../../..
	include $(top_builddir)/src/Makefile.global
	include $(top_srcdir)/contrib/contrib-global.mk
endif
