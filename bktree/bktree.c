#include "postgres.h"

#include "access/spgist.h"
#include "access/htup.h"
#include "executor/executor.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "utils/geo_decls.h"
#include "utils/array.h"
#include "utils/rel.h"

typedef struct
{
	int index;
	double distance;
} PicksplitDistanceItem;

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

#define fprintf_to_ereport(msg, ...)  ereport(NOTICE, (errmsg_internal(msg, ##__VA_ARGS__)))
// #define fprintf_to_ereport(msg, ...)


static double getDistance(Datum d1, Datum d2);
static int picksplitDistanceItemCmp(const void *v1, const void *v2);
PicksplitDistanceItem *getSplitParams(spgPickSplitIn *in, int splitIndex, double *val1, double *val2);


static double
getDistance(Datum v1, Datum v2)
{
	int64_t a1 = DatumGetInt64(v1);
	int64_t a2 = DatumGetInt64(v2);
	int64_t diff = Abs(a1 - a2);

	// fprintf_to_ereport("getDistance %ld <-> %ld : %ld", a1, a2, diff);
	return diff;
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
	double distance;
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

	distance = getDistance(in->prefixDatum, in->datum);
	out->result.matchNode.nodeN = in->nNodes - 1;
	for (i = 1; i < in->nNodes; i++)
	{
		if (distance < DatumGetFloat8(in->nodeLabels[i]))
		{
			out->result.matchNode.nodeN = i - 1;
			break;
		}
	}

	PG_RETURN_VOID();
}

PicksplitDistanceItem *
getSplitParams(spgPickSplitIn *in, int splitIndex, double *val1, double *val2)
{
	Datum splitDatum = in->datums[splitIndex];
	PicksplitDistanceItem *items = (PicksplitDistanceItem *)palloc(in->nTuples * sizeof(PicksplitDistanceItem));

	int i;
	int sameCount;

	double prevDistance;
	double distance;
	double delta;
	double avg;
	double stdDev;

	for (i = 0; i < in->nTuples; i++)
	{
		items[i].index = i;
		if (i == splitIndex)
			items[i].distance = 0.0;
		else
			items[i].distance = getDistance(splitDatum, in->datums[i]);
	}

	qsort(items, in->nTuples, sizeof(PicksplitDistanceItem), picksplitDistanceItemCmp);



	*val1 = 0.0;
	sameCount = 1;
	prevDistance = items[0].distance;
	avg = 0.0;
	for (i = 1; i < in->nTuples; i++)
	{
		distance = items[i].distance;
		if (distance != prevDistance)
		{
			*val1 += sameCount * sameCount;
			sameCount = 1;
		}
		else
		{
			sameCount++;
		}

		delta = distance - prevDistance;
		avg += delta;

		prevDistance = distance;
	}
	*val1 += sameCount * sameCount;
	avg = avg / (in->nTuples - 1);

	stdDev = 0.0;
	prevDistance = items[0].distance;
	for (i = 1; i < in->nTuples; i++)
	{
		distance = items[i].distance;
		delta = distance - prevDistance;
		stdDev += (delta - avg) * (delta - avg);
		prevDistance = distance;
	}
	stdDev = sqrt(stdDev / (in->nTuples - 2));
	*val2 = stdDev / avg;
	return items;
}

Datum
bktree_picksplit(PG_FUNCTION_ARGS)
{
	spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0);
	spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1);
	bool first = true;
	double bestVal1 = 0.0;
	double bestVal2 = 0.0;
	double nodeStartDistance;
	double nodeEndDistance;
	double prevDistance;
	int nodeStartIndex;
	int nodeEndIndex;
	PicksplitDistanceItem *bestItems = NULL;
	int i;
	int j;
	int optimalNodeSize;
	int bestIndex = 0;
	int k;

	optimalNodeSize = (in->nTuples + 7) / 8;

	fprintf_to_ereport("bktree_picksplit across %d tuples, with an optimal node size of %d", in->nTuples, optimalNodeSize);

	for (i = 0; (i < 10) && (i < in->nTuples); i++)
	{
		PicksplitDistanceItem *items;
		double val1, val2;

		items = getSplitParams(in, i, &val1, &val2);

		fprintf_to_ereport("SplitParams: (%f, %f), index: %i", bestVal1, bestVal2, i);

		if (first || val1 < bestVal1 || (val1 == bestVal1 && val2 < bestVal2))
		{
			fprintf_to_ereport("Selected item");
			first = false;
			if (bestItems)
				pfree(bestItems);
			bestItems = items;
			bestIndex = i;
			bestVal1 = val1;
			bestVal2 = val2;
		}
	}

	fprintf_to_ereport("bktree_picksplit extracted split params: (%f, %f), index: %i", bestVal1, bestVal2, bestIndex);
	fprintf_to_ereport("Picksplit best datum value: %ld", DatumGetInt64(in->datums[bestIndex]));

	out->hasPrefix = true;
	Datum tmp = Int64GetDatum(DatumGetInt64(in->datums[bestIndex]));
	out->prefixDatum = tmp;
	fprintf_to_ereport("Get data value: %ld", DatumGetInt64(in->datums[bestIndex]));
	fprintf_to_ereport("Reallocated datum (tmp): %ld", tmp);
	fprintf_to_ereport("Reallocated datum: %ld", out->prefixDatum);

	out->mapTuplesToNodes = palloc(sizeof(int)   * in->nTuples);
	out->leafTupleDatums  = palloc(sizeof(Datum) * in->nTuples);
	out->nodeLabels       = palloc(sizeof(Datum) * in->nTuples);
	out->nNodes = 0;

	nodeStartIndex = 0;
	nodeStartDistance = 0.0;
	nodeEndIndex = 0;
	nodeEndDistance = 0.0;

	out->nodeLabels[out->nNodes] = Float8GetDatum(nodeStartDistance);
	out->nNodes++;

	prevDistance = 0.0;

	fprintf_to_ereport("Splitting into %d nodes", in->nTuples / optimalNodeSize);

	for (i = 0; i < in->nTuples; i++)
	{
		double distance;
		distance = bestItems[i].distance;
		if (distance > prevDistance)
		{
			if (i - nodeStartIndex < optimalNodeSize)
			{
				nodeEndIndex = i;
				nodeEndDistance = distance;
			}
			else
			{
				fprintf_to_ereport("Split at node index %d, distance %f, nodeStartIndex: %d, current node-size: %d", i, distance, nodeStartIndex, i - nodeStartIndex);
				if (Abs(nodeEndIndex - nodeStartIndex - optimalNodeSize) >
					Abs(i - nodeStartIndex) - optimalNodeSize)
				{
					nodeEndIndex = i;
					nodeEndDistance = distance;
				}
				for (j = nodeStartIndex; j < nodeEndIndex; j++)
				{
					k = bestItems[j].index;
					out->mapTuplesToNodes[k] = out->nNodes - 1;
					out->leafTupleDatums[k]  = in->datums[k];
				}
				nodeStartIndex = nodeEndIndex;
				nodeStartDistance = nodeEndDistance;
				out->nodeLabels[out->nNodes] = Float8GetDatum(nodeStartDistance);
				out->nNodes++;
			}
		}
	}
	for (j = nodeStartIndex; j < in->nTuples; j++)
	{
		k = bestItems[j].index;
		out->mapTuplesToNodes[k] = out->nNodes - 1;
		out->leafTupleDatums[k] = in->datums[k];
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
	double queryDistance;
	double distance;
	bool isNull;
	int		i;

	fprintf_to_ereport("bktree_inner_consistent");

	out->nodeNumbers = (int *) palloc(sizeof(int) * in->nNodes);


	for (i = 0; i < in->nkeys; i++)
	{
		// The argument is a instance of bktree_area
		HeapTupleHeader query = DatumGetHeapTupleHeader(in->scankeys[i].sk_argument);
		queryDatum = GetAttributeByNum(query, 1, &isNull);
		queryDistance = DatumGetFloat8(GetAttributeByNum(query, 2, &isNull));

		Assert(in->hasPrefix);

		distance = getDistance(in->prefixDatum, queryDatum);


		if (in->allTheSame)
		{
			/* Report that all nodes should be visited */
			out->nNodes = in->nNodes;
			for (i = 0; i < in->nNodes; i++)
				out->nodeNumbers[i] = i;
			PG_RETURN_VOID();
		}

		out->nNodes = 0;
		for (i = 0; i < in->nNodes; i++)
		{
			double minDistance, maxDistance;
			bool consistent = true;

			minDistance = DatumGetFloat8(in->nodeLabels[i]);
			if (distance + queryDistance < minDistance)
			{
				consistent = false;
			}
			else if (i < in->nNodes - 1)
			{
				maxDistance = DatumGetFloat8(in->nodeLabels[i + 1]);
				if (maxDistance + queryDistance <= distance)
					consistent = false;
			}

			if (consistent)
			{
				out->nodeNumbers[out->nNodes] = i;
				out->nNodes++;
			}
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


	double distance;
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
					double queryDistance;
					bool isNull;

					fprintf_to_ereport("bktree_inner_consistent RTContainedByStrategyNumber");

					query = DatumGetHeapTupleHeader(in->scankeys[i].sk_argument);
					queryDatum = GetAttributeByNum(query, 1, &isNull);
					queryDistance = DatumGetFloat8(GetAttributeByNum(query, 2, &isNull));

					distance = getDistance(in->leafDatum, queryDatum);

					res = (distance <= queryDistance);
				}
				break;

			case RTOverLeftStrategyNumber:
				// For the equal operator, the two parameters are both int8,
				// so we just get the distance, and check if it's zero

					fprintf_to_ereport("bktree_inner_consistent RTEqualStrategyNumber");
				distance = getDistance(in->leafDatum, in->scankeys[i].sk_argument);
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
	double queryDistance;
	double distance;
	bool isNull;

	queryDatum = GetAttributeByNum(query, 1, &isNull);
	queryDistance = DatumGetFloat8(GetAttributeByNum(query, 2, &isNull));

	distance = getDistance(value, queryDatum);

	if (distance <= queryDistance)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

Datum
bktree_eq_match(PG_FUNCTION_ARGS)
{
	int64_t value_1 = DatumGetInt64(0);
	int64_t value_2 = DatumGetInt64(1);
	if (value_1 == value_2)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

Datum
bktree_get_distance(PG_FUNCTION_ARGS)
{
	PG_RETURN_FLOAT8(getDistance(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1)));
}
