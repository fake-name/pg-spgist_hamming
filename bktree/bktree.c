#include "postgres.h"

#include "access/spgist.h"
#include "access/htup.h"
#include "executor/executor.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "utils/geo_decls.h"
#include "utils/array.h"
#include "utils/rel.h"

#include "bk_tree_debug_func.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(bktree_config);
PG_FUNCTION_INFO_V1(bktree_eq_match);
PG_FUNCTION_INFO_V1(bktree_choose);
PG_FUNCTION_INFO_V1(bktree_picksplit);
PG_FUNCTION_INFO_V1(bktree_inner_consistent);
PG_FUNCTION_INFO_V1(bktree_leaf_consistent);
PG_FUNCTION_INFO_V1(bktree_area_match);
PG_FUNCTION_INFO_V1(bktree_get_distance);

Datum bktree_config(PG_FUNCTION_ARGS);
Datum bktree_choose(PG_FUNCTION_ARGS);
Datum bktree_picksplit(PG_FUNCTION_ARGS);
Datum bktree_inner_consistent(PG_FUNCTION_ARGS);
Datum bktree_leaf_consistent(PG_FUNCTION_ARGS);
Datum bktree_area_match(PG_FUNCTION_ARGS);

Datum bktree_get_distance(PG_FUNCTION_ARGS);


static int picksplitDistanceItemCmp(const void *v1, const void *v2);
PicksplitDistanceItem *getSplitParams(spgPickSplitIn *in, int splitIndex, int64_t *val1, int64_t *val2);


static int64_t
f_hamming(int64_t a_int, int64_t b_int)
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
	uint64_t ret = __builtin_popcountll (x);

	// fprintf_to_ereport("f_hamming(%ld <-> %ld): %ld", a_int, b_int, ret);
	return ret;

}

static int
picksplitDistanceItemCmp(const void *v1, const void *v2)
{
	const PicksplitDistanceItem *i1 = (const PicksplitDistanceItem *)v1;
	const PicksplitDistanceItem *i2 = (const PicksplitDistanceItem *)v2;

	if (i1->distance < i2->distance)
		return -1;
	else if (i1->distance == i2->distance)
		return 0;
	else
		return 1;
}

Datum
bktree_config(PG_FUNCTION_ARGS)
{
	/* spgConfigIn *cfgin = (spgConfigIn *) PG_GETARG_POINTER(0); */
	spgConfigOut *cfg = (spgConfigOut *) PG_GETARG_POINTER(1);

	cfg->prefixType    = POINTOID;
	cfg->labelType     = FLOAT8OID;	/* we don't need node labels */
	cfg->canReturnData = true;
	cfg->longValuesOK  = false;
	PG_RETURN_VOID();
}

Datum
bktree_choose(PG_FUNCTION_ARGS)
{
	spgChooseIn   *in = (spgChooseIn *) PG_GETARG_POINTER(0);
	spgChooseOut *out = (spgChooseOut *) PG_GETARG_POINTER(1);
	int64_t distance;
	int i;

	out->resultType = spgMatchNode;
	out->result.matchNode.levelAdd  = 0;
	out->result.matchNode.restDatum = in->datum;

	if (in->allTheSame)
	{
		/* nodeN will be set by core */
		PG_RETURN_VOID();
	}

	Assert(in->hasPrefix);

	distance = f_hamming(DatumGetInt64(in->prefixDatum), DatumGetInt64(in->datum));
	out->result.matchNode.nodeN = in->nNodes - 1;
	for (i = 1; i < in->nNodes; i++)
	{
		if (distance < DatumGetInt64(in->nodeLabels[i]))
		{
			out->result.matchNode.nodeN = i - 1;
			break;
		}
	}

	PG_RETURN_VOID();
}


Datum
bktree_picksplit(PG_FUNCTION_ARGS)
{
	spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0);
	spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1);
	Datum tmp;
	int i;

	// Since the concept of "best" isn't really a thing with BK-trees,
	// we just pick one of the input nodes at random to assign as the node
	// hash, against which to sort the child items.
	// TODO: Rewrite this to chose the input datum
	//       that produces the most child-node branches.
	int bestIndex = in->nTuples / 2;
	int64_t this_node_hash = DatumGetInt64(in->datums[bestIndex]);

	fprintf_to_ereport("bktree_picksplit across %d tuples, with child-node count of %d, on value %016x", in->nTuples, bestIndex, this_node_hash);

	out->hasPrefix = true;
	out->prefixDatum = in->datums[bestIndex];
	fprintf_to_ereport("Get data value: %ld, %ld, %ld", DatumGetInt64(in->datums[bestIndex]), tmp, out->prefixDatum);

	out->mapTuplesToNodes = palloc(sizeof(int)   * in->nTuples);
	out->leafTupleDatums  = palloc(sizeof(Datum) * in->nTuples);
	// out->nodeLabels       = palloc(sizeof(Datum) * in->nTuples);
	out->nodeLabels       = NULL;

	// Allow edit distances of 0 - 64 inclusive
	// since SP-GiST cannot store items on inner tuples, we
	// have to allow an edit-distance of zero for the pointer
	// to the leaf node containing the hash the innter-tuple is
	// labeled with.
	out->nNodes = 65;

	for (i = 0; i < in->nTuples; i++)
	{
		Datum current = in->datums[i];
		int64_t datum_hash = DatumGetInt64(current);
		int distance = f_hamming(datum_hash, this_node_hash);

		Assert(distance >= 0);
		Assert(distance <= 64);

		out->leafTupleDatums[i]  = in->datums[i];
		out->mapTuplesToNodes[i] = distance;
	}

	fprintf_to_ereport("out->nNodes %d", out->nNodes);
	PG_RETURN_VOID();
}

Datum
bktree_inner_consistent(PG_FUNCTION_ARGS)
{
	spgInnerConsistentIn *in = (spgInnerConsistentIn *) PG_GETARG_POINTER(0);
	spgInnerConsistentOut *out = (spgInnerConsistentOut *) PG_GETARG_POINTER(1);
	Datum queryDatum;
	int64_t queryDistance;
	int64_t distance;
	bool isNull;
	int		i;

	int minDistance;
	int maxDistance;

	fprintf_to_ereport("bktree_inner_consistent");

	out->nodeNumbers = (int *) palloc(sizeof(int) * in->nNodes);

	// I don't know if this is zeroed automatically
	out->nNodes = 0;

	for (i = 0; i < in->nkeys; i++)
	{
		// The argument is a instance of bktree_area
		HeapTupleHeader query = DatumGetHeapTupleHeader(in->scankeys[i].sk_argument);
		queryDatum = GetAttributeByNum(query, 1, &isNull);
		queryDistance = DatumGetFloat8(GetAttributeByNum(query, 2, &isNull));

		Assert(in->hasPrefix);

		distance = f_hamming(DatumGetInt64(in->prefixDatum), DatumGetInt64(queryDatum));
		// We want to proceed down into child-nodes that are at distances
		// hamming(search_hash, node_hash) - search distance -> hamming(search_hash, node_hash) + search distance
		// from the current node.
		// As such, we only need to perform one distance op, and just report to search
		// the relevant nodes.

		if (in->allTheSame)
		{
			/* Report that all nodes should be visited */
			out->nNodes = in->nNodes;
			for (i = 0; i < in->nNodes; i++)
				out->nodeNumbers[i] = i;
			PG_RETURN_VOID();
		}

		out->nNodes = 0;

		minDistance = max(distance-queryDistance,  0);
		maxDistance = min(distance+queryDistance, 64);

		for (i = minDistance; i < maxDistance; i++)
		{
			out->nNodes++;
			out->nodeNumbers[out->nNodes] = i;
		}
	}
	PG_RETURN_VOID();
}


Datum
bktree_leaf_consistent(PG_FUNCTION_ARGS)
{
	spgLeafConsistentIn *in = (spgLeafConsistentIn *) PG_GETARG_POINTER(0);
	spgLeafConsistentOut *out = (spgLeafConsistentOut *) PG_GETARG_POINTER(1);
	// HeapTupleHeader query = DatumGetHeapTupleHeader(in->query);


	int64_t distance;
	bool		res;
	int			i;

	res = false;
	out->recheck = false;
	out->leafValue = in->leafDatum;

	fprintf_to_ereport("bktree_leaf_consistent with %d keys", in->nkeys);

	for (i = 0; i < in->nkeys; i++)
	{
		// The argument is a instance of bktree_area
		HeapTupleHeader query;

		switch (in->scankeys[i].sk_strategy)
		{
			case RTLeftStrategyNumber:
				// For the contained parameter, we check if the distance between the target and the current
				// value is within the scope dictated by the filtering parameter
				{

					Datum queryDatum;
					int64_t queryDistance;
					bool isNull;

					fprintf_to_ereport("bktree_inner_consistent RTContainedByStrategyNumber");

					query = DatumGetHeapTupleHeader(in->scankeys[i].sk_argument);
					queryDatum = GetAttributeByNum(query, 1, &isNull);
					queryDistance = DatumGetFloat8(GetAttributeByNum(query, 2, &isNull));

					distance = f_hamming(DatumGetInt64(in->leafDatum), DatumGetInt64(queryDatum));

					res = (distance <= queryDistance);
				}
				break;

			case RTOverLeftStrategyNumber:
				// For the equal operator, the two parameters are both int8,
				// so we just get the distance, and check if it's zero

					fprintf_to_ereport("bktree_inner_consistent RTEqualStrategyNumber");
				distance = f_hamming(DatumGetInt64(in->leafDatum), DatumGetInt64(in->scankeys[i].sk_argument));
				res = (distance == 0);
				break;

			default:
				elog(ERROR, "unrecognized strategy number: %d", in->scankeys[i].sk_strategy);
				break;
		}

		if (!res)
		{
			break;
		}
	}
	PG_RETURN_BOOL(res);
}

Datum
bktree_area_match(PG_FUNCTION_ARGS)
{
	Datum value = PG_GETARG_DATUM(0);
	HeapTupleHeader query = PG_GETARG_HEAPTUPLEHEADER(1);
	Datum queryDatum;
	int64_t queryDistance;
	int64_t distance;
	bool isNull;

	queryDatum = GetAttributeByNum(query, 1, &isNull);
	queryDistance = DatumGetFloat8(GetAttributeByNum(query, 2, &isNull));

	distance = f_hamming(DatumGetInt64(value), DatumGetInt64(queryDatum));

	if (distance <= queryDistance)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

Datum
bktree_eq_match(PG_FUNCTION_ARGS)
{
	int64_t value_1 = PG_GETARG_INT64(0);
	int64_t value_2 = PG_GETARG_INT64(1);
	if (value_1 == value_2)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

Datum
bktree_get_distance(PG_FUNCTION_ARGS)
{

	int64_t value_1 = PG_GETARG_INT64(0);
	int64_t value_2 = PG_GETARG_INT64(1);

	PG_RETURN_INT64(f_hamming(value_1, value_2));
}
