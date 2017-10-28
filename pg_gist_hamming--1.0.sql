/* contrib/pg_gist_hamming/pg_gist_hamming--1.2.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_gist_hamming" to load this file. \quit


CREATE FUNCTION gbt_decompress(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbtreekey4_in(cstring)
RETURNS gbtreekey4
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbtreekey4_out(gbtreekey4)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE gbtreekey4 (
	INTERNALLENGTH = 4,
	INPUT  = gbtreekey4_in,
	OUTPUT = gbtreekey4_out
);

CREATE FUNCTION gbtreekey8_in(cstring)
RETURNS gbtreekey8
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbtreekey8_out(gbtreekey8)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE gbtreekey8 (
	INTERNALLENGTH = 8,
	INPUT  = gbtreekey8_in,
	OUTPUT = gbtreekey8_out
);

CREATE FUNCTION gbtreekey16_in(cstring)
RETURNS gbtreekey16
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbtreekey16_out(gbtreekey16)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE gbtreekey16 (
	INTERNALLENGTH = 16,
	INPUT  = gbtreekey16_in,
	OUTPUT = gbtreekey16_out
);

CREATE FUNCTION gbtreekey32_in(cstring)
RETURNS gbtreekey32
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbtreekey32_out(gbtreekey32)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE gbtreekey32 (
	INTERNALLENGTH = 32,
	INPUT  = gbtreekey32_in,
	OUTPUT = gbtreekey32_out
);

CREATE FUNCTION gbtreekey_var_in(cstring)
RETURNS gbtreekey_var
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbtreekey_var_out(gbtreekey_var)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE gbtreekey_var (
	INTERNALLENGTH = VARIABLE,
	INPUT  = gbtreekey_var_in,
	OUTPUT = gbtreekey_var_out,
	STORAGE = EXTENDED
);

--distance operators

CREATE FUNCTION int8_dist(int8, int8)
RETURNS int8
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR <-> (
	LEFTARG = int8,
	RIGHTARG = int8,
	PROCEDURE = int8_dist,
	COMMUTATOR = '<->'
);



--
--
--
-- int8 ops
--
--
--
-- define the GiST support methods
CREATE FUNCTION gbt_int8_consistent(internal,int8,int2,oid,internal)
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbt_int8_distance(internal,int8,int2,oid,internal)
RETURNS float8
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbt_int8_compress(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbt_int8_fetch(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbt_int8_penalty(internal,internal,internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbt_int8_picksplit(internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbt_int8_union(internal, internal)
RETURNS gbtreekey16
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbt_int8_same(gbtreekey16, gbtreekey16, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_int8_ops
DEFAULT FOR TYPE int8 USING gist
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_int8_consistent (internal, int8, int2, oid, internal),
	FUNCTION	2	gbt_int8_union (internal, internal),
	FUNCTION	3	gbt_int8_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_int8_penalty (internal, internal, internal),
	FUNCTION	6	gbt_int8_picksplit (internal, internal),
	FUNCTION	7	gbt_int8_same (gbtreekey16, gbtreekey16, internal),
	STORAGE		gbtreekey16;

ALTER OPERATOR FAMILY gist_int8_ops USING gist ADD
	OPERATOR	6	<> (int8, int8) ,
	OPERATOR	15	<-> (int8, int8) FOR ORDER BY pg_catalog.integer_ops ,
	FUNCTION	8 (int8, int8) gbt_int8_distance (internal, int8, int2, oid, internal) ,
	FUNCTION	9 (int8, int8) gbt_int8_fetch (internal) ;



