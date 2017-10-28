/*
 * contrib/btree_gist/btree_gist.c
 */
#include "postgres.h"

#include "btree_gist.h"
#include "btree_utils_num.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(gbt_decompress);
PG_FUNCTION_INFO_V1(gbtreekey_in);
PG_FUNCTION_INFO_V1(gbtreekey_out);

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

#define GET_FLOAT_DISTANCE(t, arg1, arg2)	Abs( ((float8) *((const t *) (arg1))) - ((float8) *((const t *) (arg2))) )


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
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	int64		query = PG_GETARG_INT64(1);

	/* Oid		subtype = PG_GETARG_OID(3); */
	int64KEY   *kkk = (int64KEY *) DatumGetPointer(entry->key);
	GBT_NUMKEY_R key;

	key.lower = (GBT_NUMKEY *) &kkk->lower;
	key.upper = (GBT_NUMKEY *) &kkk->upper;

	PG_RETURN_FLOAT8(
					 gbt_num_distance(&key, (void *) &query, GIST_LEAF(entry), &tinfo, fcinfo->flinfo)
		);
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
