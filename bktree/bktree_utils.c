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
	/*
	Convenience function, mostly for debugging.

	Takes a single in64, and returns a literal ascii string
	containing a binary representation of the passed value,
	e.g. "0111010001010....".

	This function properly handles 2's complement, so
	negative values will work properly.

	*/

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
	/*
	Convenience function, mostly for debugging.

	Takes a single literal ascii string that is 64 characters
	long, and consists of the literal representation of
	a 64-bit integer (e.g. "0111010001010...."), and
	returns an int64 that corresponds to the passed
	binary literal.

	Note that the returned value is signed, so if the MSB
	of the passed string is "1", the return value will
	be negative.

	*/

	char* arg;
	uint64_t ret = 0;
	int idx;

	arg = PG_GETARG_CSTRING(0);

	if (strlen(arg) != BITSTRING_LEN_BITS)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("bitstring_to_int64() input MUST be 64 characters long. Passed string length: %ld\n",
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
