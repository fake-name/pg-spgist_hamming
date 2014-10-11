/*
 * contrib/btree_gist/btree_utils_num.c
 */
#include "postgres.h"

#include "btree_ham_gist.h"
#include "btree_ham_utils_num.h"


GISTENTRY *
gbt_num_compress(GISTENTRY *retval, GISTENTRY *entry, const gbtree_ninfo *tinfo)
{
	if (entry->leafkey)
	{

		int64 val;

		GBT_NUMKEY *r = (GBT_NUMKEY *) palloc0(tinfo->indexsize);
		void	   *leaf = NULL;

		switch (tinfo->t)
		{
			case gbt_t_int8:
				val = DatumGetInt64(entry->key);
				leaf = &val;
				break;
			default:
				leaf = DatumGetPointer(entry->key);
		}

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
** The GiST union method for numerical values
*/

void *
gbt_num_union(GBT_NUMKEY *out, const GistEntryVector *entryvec, const gbtree_ninfo *tinfo)
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
		if ((*tinfo->f_gt) (o.lower, c.lower))	/* out->lower > cur->lower */
			memcpy((void *) o.lower, (void *) c.lower, tinfo->size);
		if ((*tinfo->f_lt) (o.upper, c.upper))	/* out->upper < cur->upper */
			memcpy((void *) o.upper, (void *) c.upper, tinfo->size);
	}

	return out;
}



/*
** The GiST same method for numerical values
*/

bool
gbt_num_same(const GBT_NUMKEY *a, const GBT_NUMKEY *b, const gbtree_ninfo *tinfo)
{
	GBT_NUMKEY_R b1,
				b2;

	b1.lower = &(((GBT_NUMKEY *) a)[0]);
	b1.upper = &(((GBT_NUMKEY *) a)[tinfo->size]);
	b2.lower = &(((GBT_NUMKEY *) b)[0]);
	b2.upper = &(((GBT_NUMKEY *) b)[tinfo->size]);



	if (
		(*tinfo->f_eq) (b1.lower, b2.lower) &&
		(*tinfo->f_eq) (b1.upper, b2.upper)
		)
	{
		return TRUE;
	}

	return FALSE;

}


void
gbt_num_bin_union(Datum *u, GBT_NUMKEY *e, const gbtree_ninfo *tinfo)
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
		if ((*tinfo->f_gt) ((void *) ur.lower, (void *) rd.lower))
			memcpy((void *) ur.lower, (void *) rd.lower, tinfo->size);
		if ((*tinfo->f_lt) ((void *) ur.upper, (void *) rd.upper))
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
				   const gbtree_ninfo *tinfo)
{
	bool		retval;

	switch (*strategy)
	{
		case BTLessEqualStrategyNumber:
			retval = (*tinfo->f_ge) (query, key->lower);
			break;
		case BTLessStrategyNumber:
			if (is_leaf)
				retval = (*tinfo->f_gt) (query, key->lower);
			else
				retval = (*tinfo->f_ge) (query, key->lower);
			break;
		case BTEqualStrategyNumber:
			if (is_leaf)
				retval = (*tinfo->f_eq) (query, key->lower);
			else
				retval = ((*tinfo->f_le) (key->lower, query) && (*tinfo->f_le) (query, key->upper)) ? true : false;
			break;
		case BTGreaterStrategyNumber:
			if (is_leaf)
				retval = (*tinfo->f_lt) (query, key->upper);
			else
				retval = (*tinfo->f_le) (query, key->upper);
			break;
		case BTGreaterEqualStrategyNumber:
			retval = (*tinfo->f_le) (query, key->upper);
			break;
		case BtreeGistNotEqualStrategyNumber:
			retval = (!((*tinfo->f_eq) (query, key->lower) &&
						(*tinfo->f_eq) (query, key->upper))) ? true : false;
			break;
		default:
			retval = false;
	}

	return (retval);
}


/*
** The GiST distance method (for KNN-Gist)
*/

float8
gbt_num_distance(const GBT_NUMKEY_R *key,
				 const void *query,
				 bool is_leaf,
				 const gbtree_ninfo *tinfo)
{
	float8		retval;

	if (tinfo->f_dist == NULL)
		elog(ERROR, "KNN search is not supported for btree_gist type %d",
			 (int) tinfo->t);
	if (tinfo->f_le(query, key->lower))
		retval = tinfo->f_dist(query, key->lower);
	else if (tinfo->f_ge(query, key->upper))
		retval = tinfo->f_dist(query, key->upper);
	else
		retval = 0.0;

	return retval;
}


GIST_SPLITVEC *
gbt_num_picksplit(const GistEntryVector *entryvec, GIST_SPLITVEC *v,
				  const gbtree_ninfo *tinfo)
{
	OffsetNumber i;
	OffsetNumber maxoff = entryvec->n - 1;
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

	elog(NOTICE, "gbt_int8_picksplit. Size = %d", sizeof(Nsrt));

	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		arr[i].t = (GBT_NUMKEY *) DatumGetPointer((entryvec->vector[i].key));
		arr[i].i = i;
	}

	qsort((void *) &arr[FirstOffsetNumber], maxoff - FirstOffsetNumber + 1, sizeof(Nsrt), tinfo->f_cmp);

	/* We do simply create two parts */

	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		if (i <= (maxoff - FirstOffsetNumber + 1) / 2)
		{
			gbt_num_bin_union(&v->spl_ldatum, arr[i].t, tinfo);
			v->spl_left[v->spl_nleft] = arr[i].i;
			v->spl_nleft++;
		}
		else
		{
			gbt_num_bin_union(&v->spl_rdatum, arr[i].t, tinfo);
			v->spl_right[v->spl_nright] = arr[i].i;
			v->spl_nright++;
		}
	}

	return v;
}



// ############################################################################################################
//
// ############################################################################################################



typedef struct int64key
{
	int64		lower;
	int64		upper;
} int64KEY;

/*
** int64 ops
*/
PG_FUNCTION_INFO_V1(gbt_int8_compress);
PG_FUNCTION_INFO_V1(gbt_int8_union);
PG_FUNCTION_INFO_V1(gbt_int8_picksplit);
PG_FUNCTION_INFO_V1(gbt_int8_consistent);
PG_FUNCTION_INFO_V1(gbt_int8_distance);
PG_FUNCTION_INFO_V1(gbt_int8_penalty);
PG_FUNCTION_INFO_V1(gbt_int8_same);


static bool
gbt_int8gt(const void *a, const void *b)
{
	return (*((const int64 *) a) > *((const int64 *) b));
}
static bool
gbt_int8ge(const void *a, const void *b)
{
	return (*((const int64 *) a) >= *((const int64 *) b));
}
static bool
gbt_int8eq(const void *a, const void *b)
{
	return (*((const int64 *) a) == *((const int64 *) b));
}
static bool
gbt_int8le(const void *a, const void *b)
{
	return (*((const int64 *) a) <= *((const int64 *) b));
}
static bool
gbt_int8lt(const void *a, const void *b)
{
	return (*((const int64 *) a) < *((const int64 *) b));
}

static int
gbt_int8key_cmp(const void *a, const void *b)
{
	int64KEY   *ia = (int64KEY *) (((const Nsrt *) a)->t);
	int64KEY   *ib = (int64KEY *) (((const Nsrt *) b)->t);

	// elog(NOTICE, "gbt_int8key_cmp = %f, %f, %f, %f", ia->lower, ib->lower, ia->upper, ib->upper);


	if (ia->lower == ib->lower)
	{
		if (ia->upper == ib->upper)
		{
			return 0;
		}

		return (ia->upper > ib->upper) ? 1 : -1;
	}


	return (ia->lower > ib->lower) ? 1 : -1;
}



int32 hamdist(int64 x, int64 y)
{
	int32 dist = 0;
	int64 val = x ^ y; // XOR

	// Count the number of set bits
	while(val)
	{
		++dist;
		val &= val - 1;
	}

	return dist;
}

static int32
gbt_int8_dist(const void *a, const void *b)
{

	return hamdist(*((const int64 *) (a)), *((const int64 *) (b)));


}


static const gbtree_ninfo tinfo =
{
	gbt_t_int8,
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

	int32		ra;

	elog(NOTICE, "int8_dist");
	ra =  hamdist(a, b);

	PG_RETURN_INT32(ra);
}


/**************************************************
 * int64 ops
 **************************************************/


Datum
gbt_int8_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	GISTENTRY  *retval = NULL;

	PG_RETURN_POINTER(gbt_num_compress(retval, entry, &tinfo));
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


	elog(NOTICE, "gbt_int8_consistent");

	/* All cases served by this function are exact */
	*recheck = false;

	key.lower = (GBT_NUMKEY *) &kkk->lower;
	key.upper = (GBT_NUMKEY *) &kkk->upper;

	PG_RETURN_BOOL(
				   gbt_num_consistent(&key, (void *) &query, &strategy, GIST_LEAF(entry), &tinfo)
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

	elog(NOTICE, "gbt_int8_distance");

	key.lower = (GBT_NUMKEY *) &kkk->lower;
	key.upper = (GBT_NUMKEY *) &kkk->upper;

	PG_RETURN_FLOAT8(
			gbt_num_distance(&key, (void *) &query, GIST_LEAF(entry), &tinfo)
		);
}


Datum
gbt_int8_union(PG_FUNCTION_ARGS)
{
	GistEntryVector *entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);
	void	   *out = palloc(sizeof(int64KEY));

	*(int *) PG_GETARG_POINTER(1) = sizeof(int64KEY);
	PG_RETURN_POINTER(gbt_num_union((void *) out, entryvec, &tinfo));
}


Datum
gbt_int8_penalty(PG_FUNCTION_ARGS)
{
	int64KEY   *origentry = (int64KEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(0))->key);
	int64KEY   *newentry = (int64KEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(1))->key);
	float	   *result = (float *) PG_GETARG_POINTER(2);


	/*
	 * Note: The factor 0.49 in following macro avoids floating point overflows
	 */

	double	tmp = 0.0F;
	(*(result))	= 0.0F;
	if ( (newentry->upper) > (origentry->upper) )
	{
		tmp += ( ((double)newentry->upper)*0.49F - ((double)origentry->upper)*0.49F );
	}
	if (	(origentry->lower) > (newentry->lower) )
	{
		tmp += ( ((double)origentry->lower)*0.49F - ((double)newentry->lower)*0.49F );
	}
	if (tmp > 0.0F)
	{
		(*(result)) += FLT_MIN;
		(*(result)) += (float) ( ((double)(tmp)) / ( (double)(tmp) + ( ((double)(origentry->upper))*0.49F - ((double)(origentry->lower))*0.49F ) ) );
		(*(result)) *= (FLT_MAX / (((GISTENTRY *) PG_GETARG_POINTER(0))->rel->rd_att->natts + 1));
	}


	// elog(NOTICE, "gbt_int8_penalty = %f", *result);

	PG_RETURN_POINTER(result);
}

Datum
gbt_int8_picksplit(PG_FUNCTION_ARGS)
{

	PG_RETURN_POINTER(gbt_num_picksplit(
									(GistEntryVector *) PG_GETARG_POINTER(0),
									  (GIST_SPLITVEC *) PG_GETARG_POINTER(1),
										&tinfo
										));
}

Datum
gbt_int8_same(PG_FUNCTION_ARGS)
{

	int64KEY   *b1 = (int64KEY *) PG_GETARG_POINTER(0);
	int64KEY   *b2 = (int64KEY *) PG_GETARG_POINTER(1);
	bool	   *result = (bool *) PG_GETARG_POINTER(2);

	*result = gbt_num_same((void *) b1, (void *) b2, &tinfo);
	PG_RETURN_POINTER(result);
}

