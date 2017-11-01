/*
 * contrib/btree_gist/btree_gist.c
 */
#include "postgres.h"

#include "btree_gist.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(gbt_decompress);
PG_FUNCTION_INFO_V1(gbtreekey_in);
PG_FUNCTION_INFO_V1(gbtreekey_out);


/*
** int64 ops
*/
PG_FUNCTION_INFO_V1(gbt_int8_compress);
PG_FUNCTION_INFO_V1(gbt_int8_fetch);
PG_FUNCTION_INFO_V1(gbt_int8_union);
PG_FUNCTION_INFO_V1(gbt_int8_picksplit);
PG_FUNCTION_INFO_V1(gbt_int8_consistent);
PG_FUNCTION_INFO_V1(gbt_int8_distance);
PG_FUNCTION_INFO_V1(gbt_int8_penalty);
PG_FUNCTION_INFO_V1(gbt_int8_same);

PG_FUNCTION_INFO_V1(gbt_int8_hamming_distance);


/**************************************************
 * #define crap I want to remove
 **************************************************/

#define SAMESIGN(a,b)	(((a) < 0) == ((b) < 0))

#define GET_FLOAT_DISTANCE(t, arg1, arg2)	Abs( ((float8) *((const t *) (arg1))) - ((float8) *((const t *) (arg2))) )

/**************************************************
 * Wattt
 **************************************************/



GISTENTRY *
gbt_num_compress(GISTENTRY *entry, const gbtree_ninfo *tinfo)
{
	GISTENTRY  *retval;

	if (entry->leafkey)
	{
		int64 v;

		GBT_NUMKEY *r = (GBT_NUMKEY *) palloc0(tinfo->indexsize);
		void	   *leaf = NULL;

		v = DatumGetInt64(entry->key);
		leaf = &v;

		Assert(tinfo->indexsize >= 2 * tinfo->size);

		memcpy((void *) &r[0], leaf, tinfo->size);
		memcpy((void *) &r[tinfo->size], leaf, tinfo->size);
		retval = palloc(sizeof(GISTENTRY));
		gistentryinit(*retval, PointerGetDatum(r), entry->rel, entry->page,
					  entry->offset, FALSE);
	}
	else
		retval = entry;

	return retval;
}

/*
 * Convert a compressed leaf item back to the original type, for index-only
 * scans.
 */
GISTENTRY *
gbt_num_fetch(GISTENTRY *entry, const gbtree_ninfo *tinfo)
{
	GISTENTRY  *retval;
	Datum		datum;

	Assert(tinfo->indexsize >= 2 * tinfo->size);

	/*
	 * Get the original Datum from the stored datum. On leaf entries, the
	 * lower and upper bound are the same. We just grab the lower bound and
	 * return it.
	 */

	datum = Int64GetDatum(*(int64 *) entry->key);


	retval = palloc(sizeof(GISTENTRY));
	gistentryinit(*retval, datum, entry->rel, entry->page, entry->offset,
				  FALSE);
	return retval;
}



/*
** The GiST union method for numerical values
*/

void *
gbt_num_union(GBT_NUMKEY *out, const GistEntryVector *entryvec, const gbtree_ninfo *tinfo, FmgrInfo *flinfo)
{
	int			i,
				numranges;
	GBT_NUMKEY *cur;
	GBT_NUMKEY_R o,
				c;

	numranges = entryvec->n;
	cur = (GBT_NUMKEY *) DatumGetPointer((entryvec->vector[0].key));


	o.lower = &((GBT_NUMKEY *) out)[0];
	o.upper = &((GBT_NUMKEY *) out)[tinfo->size];

	memcpy((void *) out, (void *) cur, 2 * tinfo->size);

	for (i = 1; i < numranges; i++)
	{
		cur = (GBT_NUMKEY *) DatumGetPointer((entryvec->vector[i].key));
		c.lower = &cur[0];
		c.upper = &cur[tinfo->size];
		/* if out->lower > cur->lower, adopt cur as lower */
		if (tinfo->f_gt(o.lower, c.lower, flinfo))
			memcpy((void *) o.lower, (void *) c.lower, tinfo->size);
		/* if out->upper < cur->upper, adopt cur as upper */
		if (tinfo->f_lt(o.upper, c.upper, flinfo))
			memcpy((void *) o.upper, (void *) c.upper, tinfo->size);
	}

	return out;
}



/*
** The GiST same method for numerical values
*/

bool
gbt_num_same(const GBT_NUMKEY *a, const GBT_NUMKEY *b, const gbtree_ninfo *tinfo, FmgrInfo *flinfo)
{
	GBT_NUMKEY_R b1,
				b2;

	b1.lower = &(((GBT_NUMKEY *) a)[0]);
	b1.upper = &(((GBT_NUMKEY *) a)[tinfo->size]);
	b2.lower = &(((GBT_NUMKEY *) b)[0]);
	b2.upper = &(((GBT_NUMKEY *) b)[tinfo->size]);

	return (tinfo->f_eq(b1.lower, b2.lower, flinfo) &&
			tinfo->f_eq(b1.upper, b2.upper, flinfo));
}


void
gbt_num_bin_union(Datum *u, GBT_NUMKEY *e, const gbtree_ninfo *tinfo, FmgrInfo *flinfo)
{
	GBT_NUMKEY_R rd;

	rd.lower = &e[0];
	rd.upper = &e[tinfo->size];

	if (!DatumGetPointer(*u))
	{
		*u = PointerGetDatum(palloc0(tinfo->indexsize));
		memcpy((void *) &(((GBT_NUMKEY *) DatumGetPointer(*u))[0]), (void *) rd.lower, tinfo->size);
		memcpy((void *) &(((GBT_NUMKEY *) DatumGetPointer(*u))[tinfo->size]), (void *) rd.upper, tinfo->size);
	}
	else
	{
		GBT_NUMKEY_R ur;

		ur.lower = &(((GBT_NUMKEY *) DatumGetPointer(*u))[0]);
		ur.upper = &(((GBT_NUMKEY *) DatumGetPointer(*u))[tinfo->size]);
		if (tinfo->f_gt((void *) ur.lower, (void *) rd.lower, flinfo))
			memcpy((void *) ur.lower, (void *) rd.lower, tinfo->size);
		if (tinfo->f_lt((void *) ur.upper, (void *) rd.upper, flinfo))
			memcpy((void *) ur.upper, (void *) rd.upper, tinfo->size);
	}
}



/*
 * The GiST consistent method
 *
 * Note: we currently assume that no datatypes that use this routine are
 * collation-aware; so we don't bother passing collation through.
 */
bool
gbt_num_consistent(const GBT_NUMKEY_R *key,
				   const void *query,
				   const StrategyNumber *strategy,
				   bool is_leaf,
				   const gbtree_ninfo *tinfo,
				   FmgrInfo *flinfo)
{
	bool		retval;

	switch (*strategy)
	{
		case BTLessEqualStrategyNumber:
			retval = tinfo->f_ge(query, key->lower, flinfo);
			break;
		case BTLessStrategyNumber:
			if (is_leaf)
				retval = tinfo->f_gt(query, key->lower, flinfo);
			else
				retval = tinfo->f_ge(query, key->lower, flinfo);
			break;
		case BTEqualStrategyNumber:
			if (is_leaf)
				retval = tinfo->f_eq(query, key->lower, flinfo);
			else
				retval = (tinfo->f_le(key->lower, query, flinfo) &&
						  tinfo->f_le(query, key->upper, flinfo));
			break;
		case BTGreaterStrategyNumber:
			if (is_leaf)
				retval = tinfo->f_lt(query, key->upper, flinfo);
			else
				retval = tinfo->f_le(query, key->upper, flinfo);
			break;
		case BTGreaterEqualStrategyNumber:
			retval = tinfo->f_le(query, key->upper, flinfo);
			break;
		case BtreeGistNotEqualStrategyNumber:
			retval = (!(tinfo->f_eq(query, key->lower, flinfo) &&
						tinfo->f_eq(query, key->upper, flinfo)));
			break;
		default:
			retval = false;
	}

	return retval;
}


/*
** The GiST distance method (for KNN-Gist)
*/

float8
gbt_num_distance(const GBT_NUMKEY_R *key,
				 const void *query,
				 bool is_leaf,
				 const gbtree_ninfo *tinfo,
				 FmgrInfo *flinfo)
{
	float8		retval;

	if (tinfo->f_le(query, key->lower, flinfo))
		retval = tinfo->f_dist(query, key->lower, flinfo);
	else if (tinfo->f_ge(query, key->upper, flinfo))
		retval = tinfo->f_dist(query, key->upper, flinfo);
	else
		retval = 0.0;

	return retval;
}


GIST_SPLITVEC *
gbt_num_picksplit(const GistEntryVector *entryvec, GIST_SPLITVEC *v,
				  const gbtree_ninfo *tinfo, FmgrInfo *flinfo)
{
	OffsetNumber i,
				maxoff = entryvec->n - 1;
	Nsrt	   *arr;
	int			nbytes;

	arr = (Nsrt *) palloc((maxoff + 1) * sizeof(Nsrt));
	nbytes = (maxoff + 2) * sizeof(OffsetNumber);
	v->spl_left = (OffsetNumber *) palloc(nbytes);
	v->spl_right = (OffsetNumber *) palloc(nbytes);
	v->spl_ldatum = PointerGetDatum(0);
	v->spl_rdatum = PointerGetDatum(0);
	v->spl_nleft = 0;
	v->spl_nright = 0;

	/* Sort entries */

	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		arr[i].t = (GBT_NUMKEY *) DatumGetPointer((entryvec->vector[i].key));
		arr[i].i = i;
	}
	qsort_arg((void *) &arr[FirstOffsetNumber], maxoff - FirstOffsetNumber + 1, sizeof(Nsrt), (qsort_arg_comparator) tinfo->f_cmp, (void *) flinfo);

	/* We do simply create two parts */

	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		if (i <= (maxoff - FirstOffsetNumber + 1) / 2)
		{
			gbt_num_bin_union(&v->spl_ldatum, arr[i].t, tinfo, flinfo);
			v->spl_left[v->spl_nleft] = arr[i].i;
			v->spl_nleft++;
		}
		else
		{
			gbt_num_bin_union(&v->spl_rdatum, arr[i].t, tinfo, flinfo);
			v->spl_right[v->spl_nright] = arr[i].i;
			v->spl_nright++;
		}
	}

	return v;
}



/**************************************************
 * In/Out for keys
 **************************************************/


Datum
gbtreekey_in(PG_FUNCTION_ARGS)
{
	ereport(ERROR,
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			 errmsg("<datatype>key_in() not implemented")));

	PG_RETURN_POINTER(NULL);
}

// #include "btree_utils_var.h"
#include "utils/builtins.h"
Datum
gbtreekey_out(PG_FUNCTION_ARGS)
{
	ereport(ERROR,
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			 errmsg("<datatype>key_out() not implemented")));
	PG_RETURN_POINTER(NULL);
}


/*
** GiST DeCompress methods
** do not do anything.
*/
Datum
gbt_decompress(PG_FUNCTION_ARGS)
{
	PG_RETURN_POINTER(PG_GETARG_POINTER(0));
}




typedef struct int64key
{
	int64		lower;
	int64		upper;
} int64KEY;



static bool
gbt_int8gt(const void *a, const void *b, FmgrInfo *flinfo)
{
	return (*((const int64 *) a) > *((const int64 *) b));
}
static bool
gbt_int8ge(const void *a, const void *b, FmgrInfo *flinfo)
{
	return (*((const int64 *) a) >= *((const int64 *) b));
}
static bool
gbt_int8eq(const void *a, const void *b, FmgrInfo *flinfo)
{
	return (*((const int64 *) a) == *((const int64 *) b));
}
static bool
gbt_int8le(const void *a, const void *b, FmgrInfo *flinfo)
{
	return (*((const int64 *) a) <= *((const int64 *) b));
}
static bool
gbt_int8lt(const void *a, const void *b, FmgrInfo *flinfo)
{
	return (*((const int64 *) a) < *((const int64 *) b));
}

static int
gbt_int8key_cmp(const void *a, const void *b, FmgrInfo *flinfo)
{
	int64KEY   *ia = (int64KEY *) (((const Nsrt *) a)->t);
	int64KEY   *ib = (int64KEY *) (((const Nsrt *) b)->t);

	if (ia->lower == ib->lower)
	{
		if (ia->upper == ib->upper)
			return 0;

		return (ia->upper > ib->upper) ? 1 : -1;
	}

	return (ia->lower > ib->lower) ? 1 : -1;
}



int64_t inline f_hamming(int64_t a_int, int64_t b_int)
{
	/*
	Compute number of bits that are not common between `a` and `b`.
	return value is a plain integer
	*/

	// uint64_t x = (a_int ^ b_int);
	// __asm__(
	// 	"popcnt %0 %0  \n\t"// +r means input/output, r means intput
	// 	: "+r" (x) );
	// return x;


	uint64_t x = (a_int ^ b_int);

	// Depend on GCC primitives
	return __builtin_popcountll (x);

}



static float8
gbt_int8_dist(const void *a, const void *b, FmgrInfo *flinfo)
{

	return GET_FLOAT_DISTANCE(int64, a, b);

	// return f_hamming( (*((const int64 *) (a))), (*((const int64 *) (b))) );

}



static const gbtree_ninfo tinfo =
{
	sizeof(int64),
	16,							/* sizeof(gbtreekey16) */
	gbt_int8gt,
	gbt_int8ge,
	gbt_int8eq,
	gbt_int8le,
	gbt_int8lt,
	gbt_int8key_cmp,
	gbt_int8_dist
};


PG_FUNCTION_INFO_V1(int8_dist);
Datum
int8_dist(PG_FUNCTION_ARGS)
{
	int64		a = PG_GETARG_INT64(0);
	int64		b = PG_GETARG_INT64(1);
	int64		ra;

	// ra = f_hamming(a, b);


	int64		r;

	r = a - b;
	ra = Abs(r);

	/* Overflow check. */
	if (ra < 0 || (!SAMESIGN(a, b) && !SAMESIGN(r, a)))
		ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				 errmsg("bigint out of range")));


	PG_RETURN_INT64(ra);
}


/**************************************************
 * int64 ops
 **************************************************/


Datum
gbt_int8_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);

	PG_RETURN_POINTER(gbt_num_compress(entry, &tinfo));
}

Datum
gbt_int8_fetch(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);

	PG_RETURN_POINTER(gbt_num_fetch(entry, &tinfo));
}

Datum
gbt_int8_consistent(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	int64		query = PG_GETARG_INT64(1);
	StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);

	/* Oid		subtype = PG_GETARG_OID(3); */
	bool	   *recheck = (bool *) PG_GETARG_POINTER(4);
	int64KEY   *kkk = (int64KEY *) DatumGetPointer(entry->key);
	GBT_NUMKEY_R key;

	/* All cases served by this function are exact */
	*recheck = false;

	key.lower = (GBT_NUMKEY *) &kkk->lower;
	key.upper = (GBT_NUMKEY *) &kkk->upper;

	PG_RETURN_BOOL(
					 gbt_num_consistent(&key, (void *) &query, &strategy, GIST_LEAF(entry), &tinfo, fcinfo->flinfo)
		);
}


Datum
gbt_int8_distance(PG_FUNCTION_ARGS)
{
	double ret;

	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	int64		query = PG_GETARG_INT64(1);

	/* Oid		subtype = PG_GETARG_OID(3); */
	int64KEY   *kkk = (int64KEY *) DatumGetPointer(entry->key);
	GBT_NUMKEY_R key;

	key.lower = (GBT_NUMKEY *) &kkk->lower;
	key.upper = (GBT_NUMKEY *) &kkk->upper;

	ret = gbt_num_distance(&key, (void *) &query, GIST_LEAF(entry), &tinfo, fcinfo->flinfo);

	PG_RETURN_FLOAT8(ret);
}


Datum
gbt_int8_hamming_distance(PG_FUNCTION_ARGS)
{
	double ret;

	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	int64		query = PG_GETARG_INT64(1);

	/* Oid		subtype = PG_GETARG_OID(3); */
	int64KEY   *kkk = (int64KEY *) DatumGetPointer(entry->key);
	GBT_NUMKEY_R key;

	key.lower = (GBT_NUMKEY *) &kkk->lower;
	key.upper = (GBT_NUMKEY *) &kkk->upper;


	ret = gbt_num_distance(&key, (void *) &query, GIST_LEAF(entry), &tinfo, fcinfo->flinfo);

	PG_RETURN_FLOAT8(ret);

	// return f_hamming( (*((const int64 *) (a))), (*((const int64 *) (b))) );

}


Datum
gbt_int8_union(PG_FUNCTION_ARGS)
{
	GistEntryVector *entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);
	void	   *out = palloc(sizeof(int64KEY));

	*(int *) PG_GETARG_POINTER(1) = sizeof(int64KEY);
	PG_RETURN_POINTER(gbt_num_union((void *) out, entryvec, &tinfo, fcinfo->flinfo));
}






/*
 * Note: The factor 0.49 in following macro avoids floating point overflows
 */
#define penalty_num(result,olower,oupper,nlower,nupper) do { \
	double	tmp = 0.0F; \
	(*(result))	= 0.0F; \
	if ( (nupper) > (oupper) ) \
		tmp += ( ((double)nupper)*0.49F - ((double)oupper)*0.49F ); \
	if (	(olower) > (nlower)  ) \
		tmp += ( ((double)olower)*0.49F - ((double)nlower)*0.49F ); \
	if (tmp > 0.0F) \
	{ \
	(*(result)) += FLT_MIN; \
	(*(result)) += (float) ( ((double)(tmp)) / ( (double)(tmp) + ( ((double)(oupper))*0.49F - ((double)(olower))*0.49F ) ) ); \
	(*(result)) *= (FLT_MAX / (((GISTENTRY *) PG_GETARG_POINTER(0))->rel->rd_att->natts + 1)); \
	} \
} while (0);



Datum
gbt_int8_penalty(PG_FUNCTION_ARGS)
{
	int64KEY   *origentry = (int64KEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(0))->key);
	int64KEY   *newentry = (int64KEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(1))->key);
	float	   *result = (float *) PG_GETARG_POINTER(2);

	penalty_num(result, origentry->lower, origentry->upper, newentry->lower, newentry->upper);

	PG_RETURN_POINTER(result);
}

Datum
gbt_int8_picksplit(PG_FUNCTION_ARGS)
{
	PG_RETURN_POINTER(gbt_num_picksplit(
										(GistEntryVector *) PG_GETARG_POINTER(0),
										(GIST_SPLITVEC *) PG_GETARG_POINTER(1),
										&tinfo, fcinfo->flinfo
										));
}

Datum
gbt_int8_same(PG_FUNCTION_ARGS)
{
	int64KEY   *b1 = (int64KEY *) PG_GETARG_POINTER(0);
	int64KEY   *b2 = (int64KEY *) PG_GETARG_POINTER(1);
	bool	   *result = (bool *) PG_GETARG_POINTER(2);

	*result = gbt_num_same((void *) b1, (void *) b2, &tinfo, fcinfo->flinfo);
	PG_RETURN_POINTER(result);
}
