

MODULE_big = bktree
OBJS = bktree.o bktree_utils.o

EXTENSION = bktree
DATA = bktree--1.0.sql

REGRESS = init utilities test_1 test_2 int8
# REGRESS = init int8 int8_2 # not_equal


PGFILEDESC = "bktree - Viewpoint Tree GiST Index operator"


ifndef DO_NOT_USE_PGXS
	PG_CONFIG = pg_config
	PGXS := $(shell $(PG_CONFIG) --pgxs)
	include $(PGXS)
else
	subdir = contrib/pg_gist_hamming/bktree
	top_builddir = ../../..
	include $(top_builddir)/src/Makefile.global
	include $(top_srcdir)/contrib/contrib-global.mk
endif
