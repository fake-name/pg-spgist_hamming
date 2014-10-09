# contrib/btree_gist/Makefile

MODULE_big = btree_ham_gist

OBJS =  btree_ham_gist.o btree_ham_utils_num.o btree_ham_int8.o $(WIN32RES)

EXTENSION = btree_ham_gist
DATA = btree_ham_gist--1.0.sql btree_ham_gist--unpackaged--1.0.sql
PGFILEDESC = "btree_ham_gist - B-tree equivalent GIST operator classes"

REGRESS = init int8 not_equal

SHLIB_LINK += $(filter -lm, $(LIBS))

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)