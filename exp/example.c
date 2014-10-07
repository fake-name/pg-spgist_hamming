#include "postgres.h"
#include "fmgr.h"
#include <string.h>

// example from http://linuxgazette.net/139/peterson.html
// Patched for postgres 9.3

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1( hello );

Datum hello( PG_FUNCTION_ARGS );
char *dup_pgtext(text *what);

char *dup_pgtext(text *what)
{
	char *dup;
	size_t len = 0;

	len = VARSIZE(what)-VARHDRSZ;
	dup = palloc(len+1);
	strncpy(dup, VARDATA(what), len);
	dup[len] = 0;
	return dup;
}

Datum hello( PG_FUNCTION_ARGS )
{
	// variable declarations
	char greet[] = "Hello, ";
	text *towhom;
	int greetlen = 0;
	int towhomlen = 0;
	text *greeting;

	char *prntStr;

	// Get arguments.  If we declare our function as STRICT, then
	// this check is superfluous.
	if( PG_ARGISNULL(0) ) {
		PG_RETURN_NULL();
	}
	towhom = PG_GETARG_TEXT_P(0);

	// Calculate string sizes.
	greetlen = strlen(greet);
	towhomlen = VARSIZE(towhom) - VARHDRSZ;

	// Allocate memory and set data structure size.
	greeting = (text *)palloc( greetlen + towhomlen );
	SET_VARSIZE( greeting, greetlen + towhomlen  + VARHDRSZ);

	// Construct greeting string.
	strncpy( VARDATA(greeting)           , greet          , greetlen  );
	strncpy( VARDATA(greeting) + greetlen, VARDATA(towhom), towhomlen );

	prntStr = dup_pgtext(greeting);

	ereport( INFO,
		( errcode( ERRCODE_SUCCESSFUL_COMPLETION ),
		  errmsg( "to : %s", dup_pgtext(towhom) )));

	ereport( INFO,
		( errcode( ERRCODE_SUCCESSFUL_COMPLETION ),
		  errmsg( "out: %s", prntStr )));

	pfree(prntStr);

	PG_RETURN_TEXT_P( greeting );
}