/* contrib/btree_gist/btree_gist--unpackaged--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION btree_gist" to load this file. \quit

ALTER EXTENSION btree_gist ADD type gbtreekey4;
ALTER EXTENSION btree_gist ADD function gbtreekey4_in(cstring);
ALTER EXTENSION btree_gist ADD function gbtreekey4_out(gbtreekey4);
ALTER EXTENSION btree_gist ADD type gbtreekey8;
ALTER EXTENSION btree_gist ADD function gbtreekey8_in(cstring);
ALTER EXTENSION btree_gist ADD function gbtreekey8_out(gbtreekey8);
ALTER EXTENSION btree_gist ADD type gbtreekey16;
ALTER EXTENSION btree_gist ADD function gbtreekey16_in(cstring);
ALTER EXTENSION btree_gist ADD function gbtreekey16_out(gbtreekey16);
ALTER EXTENSION btree_gist ADD type gbtreekey32;
ALTER EXTENSION btree_gist ADD function gbtreekey32_in(cstring);
ALTER EXTENSION btree_gist ADD function gbtreekey32_out(gbtreekey32);
ALTER EXTENSION btree_gist ADD type gbtreekey_var;
ALTER EXTENSION btree_gist ADD function gbtreekey_var_in(cstring);
ALTER EXTENSION btree_gist ADD function gbtreekey_var_out(gbtreekey_var);


ALTER EXTENSION btree_gist ADD function gbt_decompress(internal);
ALTER EXTENSION btree_gist ADD function gbt_var_decompress(internal);





ALTER EXTENSION btree_gist ADD function gbt_int8_consistent(internal,bigint,smallint,oid,internal);
ALTER EXTENSION btree_gist ADD function gbt_int8_compress(internal);
ALTER EXTENSION btree_gist ADD function gbt_int8_penalty(internal,internal,internal);
ALTER EXTENSION btree_gist ADD function gbt_int8_picksplit(internal,internal);
ALTER EXTENSION btree_gist ADD function gbt_int8_union(bytea,internal);
ALTER EXTENSION btree_gist ADD function gbt_int8_same(internal,internal,internal);
ALTER EXTENSION btree_gist ADD operator family gist_int8_ops using gist;
ALTER EXTENSION btree_gist ADD operator class gist_int8_ops using gist;






-- Add functions and operators that are new in 9.1

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

-- Support functions for distance operators

CREATE FUNCTION gbt_int8_distance(internal,int8,int2,oid)
RETURNS int8
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;



-- Add new-in-9.1 stuff to the operator classes.

ALTER OPERATOR FAMILY gist_int8_ops USING gist ADD
	OPERATOR	6	<> (int8, int8) ,
	OPERATOR	15	<-> (int8, int8) FOR ORDER BY pg_catalog.integer_ops ,
	FUNCTION	8 (int8, int8) gbt_int8_distance (internal, int8, int2, oid) ;
