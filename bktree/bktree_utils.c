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

typedef struct
{
	int index;
	double distance;
} PicksplitDistanceItem;


PG_FUNCTION_INFO_V1(int64_to_bitstring);
PG_FUNCTION_INFO_V1(bitstring_to_int64);

Datum int64_to_bitstring(PG_FUNCTION_ARGS);
Datum bitstring_to_int64(PG_FUNCTION_ARGS);

#define BITSTRING_LEN_BITS 64

Datum
int64_to_bitstring(PG_FUNCTION_ARGS)
{


	StringInfoData buf;
	int			i;
	uint64_t value = (uint64_t) DatumGetInt64(PG_GETARG_DATUM(0));
	uint64_t mask  = 1ULL << (BITSTRING_LEN_BITS - 1);

	initStringInfo(&buf);

	for (i = 0; i < BITSTRING_LEN_BITS; i++)
	{
		if (value & mask)
			appendStringInfoChar(&buf, '1');
		else
			appendStringInfoChar(&buf, '0');

		mask >>= 1;
	}

	PG_RETURN_CSTRING(buf.data);
}

Datum
bitstring_to_int64(PG_FUNCTION_ARGS)
{

	char* arg;
	uint64_t ret = 0;
	int idx;

	arg = PG_GETARG_CSTRING(0);

	if (strlen(arg) != BITSTRING_LEN_BITS)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("bitstring_to_int64() input MUST be 64 characters long. Passed string length: %d\n",
						strlen(arg))));
	}
	for (idx = 0; idx < BITSTRING_LEN_BITS; idx += 1)
	{
		if (arg[idx] == '1')
		{
			ret |= (1ULL << (BITSTRING_LEN_BITS - (1 + idx)));
		}
		else if (arg[idx] == '0')
		{
			// Do nothing, bit is already 0
		}
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("bitstring_to_int64() input can only contain either '1' or '0'. Found bad char '%c' at index %d\n",
					 		arg[idx], idx)));

		}
	}

	PG_RETURN_INT64(ret);

}
