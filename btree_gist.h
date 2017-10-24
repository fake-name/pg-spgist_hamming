/*
 * contrib/btree_gist/btree_gist.h
 */
#ifndef __BTREE_GIST_H__
#define __BTREE_GIST_H__

#include "fmgr.h"
#include "access/nbtree.h"

#define BtreeGistNotEqualStrategyNumber 6

/* indexed types */

enum gbtree_type
{
	gbt_t_int8
};

#endif
