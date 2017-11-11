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


#define int_min(a, b) (((a) < (b)) ? (a) : (b))
#define int_max(a, b) (((a) > (b)) ? (a) : (b))

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


Datum
bktree_config(PG_FUNCTION_ARGS)
{
	/* spgConfigIn *cfgin = (spgConfigIn *) PG_GETARG_POINTER(0); */
	spgConfigOut *cfg = (spgConfigOut *) PG_GETARG_POINTER(1);

	cfg->prefixType    = INT8OID;
	cfg->labelType     = INT8OID;	/* we don't need node labels */
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

	out->resultType = spgMatchNode;
	out->result.matchNode.levelAdd  = 0;
	out->result.matchNode.restDatum = in->datum;

	if (in->allTheSame)
	{
		/* nodeN will be set by core */
		PG_RETURN_VOID();
	}

	Assert(in->nNodes == 65);
	Assert(in->hasPrefix);

	distance = f_hamming(DatumGetInt64(in->prefixDatum), DatumGetInt64(in->datum));
	Assert(distance >= 0);
	Assert(distance <= 64);

	// The new node gets slotted into the child with the appropriate distance
	out->result.matchNode.nodeN = distance;

	PG_RETURN_VOID();
}


Datum
bktree_picksplit(PG_FUNCTION_ARGS)
{
	spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0);
	spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1);
	int i;

	// Since the concept of "best" isn't really a thing with BK-trees,
	// we just pick one of the input nodes at random to assign as the node
	// hash, against which to sort the child items.
	//
	// TODO: Rewrite this to chose the input datum
	//       that produces the most child-node branches.
	int bestIndex = in->nTuples / 2;
	int64_t this_node_hash = DatumGetInt64(in->datums[bestIndex]);

	fprintf_to_ereport("bktree_picksplit across %d tuples, with child-node count of %d (bestindex %d), on value %016x", in->nTuples, in->nTuples, bestIndex, this_node_hash);

	out->hasPrefix = true;
	out->prefixDatum = in->datums[bestIndex];
	fprintf_to_ereport("Get data value: %ld, %ld", DatumGetInt64(in->datums[bestIndex]), out->prefixDatum);

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
	int64_t queryTargetValue;
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


	if (in->allTheSame)
	{
		fprintf_to_ereport("in->allTheSame is true");
		/* Report that all nodes should be visited */
		out->nNodes = in->nNodes;
		for (i = 0; i < in->nNodes; i++)
			out->nodeNumbers[i] = i;
		PG_RETURN_VOID();
	}

	for (i = 0; i < in->nkeys; i++)
	{
		HeapTupleHeader query;
		switch (in->scankeys[i].sk_strategy)
		{
			case RTLeftStrategyNumber:
				// The argument is a instance of bktree_area
				query = DatumGetHeapTupleHeader(in->scankeys[i].sk_argument);
				queryTargetValue = DatumGetInt64(GetAttributeByNum(query, 1, &isNull));
				queryDistance = DatumGetInt64(GetAttributeByNum(query, 2, &isNull));


				Assert(in->hasPrefix);

				distance = f_hamming(DatumGetInt64(in->prefixDatum), queryTargetValue);

				fprintf_to_ereport("RTLeftStrategyNumber search for %ld with distance of %ld", queryTargetValue, queryDistance);
				fprintf_to_ereport("Nodes: current %016x, target %016x, distance %d", DatumGetInt64(in->prefixDatum), queryTargetValue, distance);

				// We want to proceed down into child-nodes that are at distances
				// hamming(search_hash, node_hash) - search distance -> hamming(search_hash, node_hash) + search distance
				// from the current node.
				// As such, we only need to perform one distance op, and just report to search
				// the relevant nodes.

				Assert(distance >= 0);
				Assert(distance <= 64);

				minDistance = int_max(distance-queryDistance,  0);
				maxDistance = int_min(distance+queryDistance, 64);

				for (i = minDistance; i <= maxDistance; i++)
				{

					fprintf_to_ereport("Out Nodes: %d, inserting node number %d", out->nNodes, i);
					out->nodeNumbers[out->nNodes] = i;
					out->nNodes++;
				}
				break;

			case RTOverLeftStrategyNumber:
				// With a search distance of 0, we just
				// calculate the node->child object distance, and return the node
				// at that distance.
				fprintf_to_ereport("bktree_inner_consistent RTEqualStrategyNumber");
				distance = f_hamming(DatumGetInt64(in->prefixDatum), DatumGetInt64(in->scankeys[i].sk_argument));

				out->nNodes++;
				out->nodeNumbers[out->nNodes] = distance;

			default:
				elog(ERROR, "unrecognized strategy number: %d", in->scankeys[i].sk_strategy);
				break;
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

					int64_t queryHash;
					int64_t queryDistance;
					bool isNull;


					query = DatumGetHeapTupleHeader(in->scankeys[i].sk_argument);
					queryHash = DatumGetInt64(GetAttributeByNum(query, 1, &isNull));
					queryDistance = DatumGetInt64(GetAttributeByNum(query, 2, &isNull));

					distance = f_hamming(DatumGetInt64(in->leafDatum), queryHash);

					fprintf_to_ereport("bktree_leaf_consistent RTContainedByStrategyNumber");
					fprintf_to_ereport("Searching for %ld with distance %ld, current node %ld, distance %d", queryHash, queryDistance, DatumGetInt64(in->leafDatum), distance);

					res = (distance <= queryDistance);
				}
				break;

			case RTOverLeftStrategyNumber:
				// For the equal operator, the two parameters are both int8,
				// so we just get the distance, and check if it's zero

				fprintf_to_ereport("bktree_leaf_consistent RTEqualStrategyNumber");
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
	queryDistance = DatumGetInt64(GetAttributeByNum(query, 2, &isNull));

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
