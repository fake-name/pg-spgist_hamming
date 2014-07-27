#include "postgres.h"
#include "fmgr.h"
/*
 * You can include more files here if needed.
 * To use some types, you must include the
 * correct file here based on:
 * http://www.postgresql.org/docs/current/static/xfunc-c.html#XFUNC-C-TYPE-TABLE
 */

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(spgist_hamming);
Datum spgist_hamming(PG_FUNCTION_ARGS);

Datum
spgist_hamming(PG_FUNCTION_ARGS)
{
	/*
	 * This is an empty body and will return NULL
	 *
	 * You should remove this comment and type
	 * cool code here!
	 */

	PG_RETURN_NULL();
}



