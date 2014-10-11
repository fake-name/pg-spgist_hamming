/*
 * contrib/btree_gist/btree_utils_num.h
 */
#ifndef __BTREE_UTILS_NUM_H__
#define __BTREE_UTILS_NUM_H__

#include "btree_ham_gist.h"
#include "access/gist.h"
#include "utils/rel.h"

#include <math.h>
#include <float.h>

typedef char GBT_NUMKEY;

/* Better readable key */
typedef struct
{
	const GBT_NUMKEY *lower,
	                 *upper;
} GBT_NUMKEY_R;


/* for sorting */
typedef struct
{
	int			i;
	GBT_NUMKEY *t;
} Nsrt;


/* type description */

typedef struct
{

	/* Attribs */

	enum gbtree_type t;			/* data type */
	int32		size;			/* size of type, 0 means variable */
	int32		indexsize;		/* size of datums stored in index */

	/* Methods */

	bool		(*f_gt) (const void *, const void *);	/* greater than */
	bool		(*f_ge) (const void *, const void *);	/* greater or equal */
	bool		(*f_eq) (const void *, const void *);	/* equal */
	bool		(*f_le) (const void *, const void *);	/* less or equal */
	bool		(*f_lt) (const void *, const void *);	/* less than */
	int			(*f_cmp) (const void *, const void *);	/* key compare function */
	int			(*f_dist) (const void *, const void *); /* key distance function */
} gbtree_ninfo;


/*
 *	Numeric btree functions
 */






#define SAMESIGN(a,b)	(((a) < 0) == ((b) < 0))


extern Interval *abs_interval(Interval *a);

extern bool gbt_num_consistent(const GBT_NUMKEY_R *key, const void *query,
				   const StrategyNumber *strategy, bool is_leaf,
				   const gbtree_ninfo *tinfo);

extern float8 gbt_num_distance(const GBT_NUMKEY_R *key, const void *query,
				 bool is_leaf, const gbtree_ninfo *tinfo);

extern GIST_SPLITVEC *gbt_num_picksplit(const GistEntryVector *entryvec, GIST_SPLITVEC *v,
				  const gbtree_ninfo *tinfo);

extern GISTENTRY *gbt_num_compress(GISTENTRY *retval, GISTENTRY *entry,
				 const gbtree_ninfo *tinfo);


extern void *gbt_num_union(GBT_NUMKEY *out, const GistEntryVector *entryvec,
			  const gbtree_ninfo *tinfo);

extern bool gbt_num_same(const GBT_NUMKEY *a, const GBT_NUMKEY *b,
			 const gbtree_ninfo *tinfo);

extern void gbt_num_bin_union(Datum *u, GBT_NUMKEY *e,
				  const gbtree_ninfo *tinfo);

#endif
