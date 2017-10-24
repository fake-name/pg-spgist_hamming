/* contrib/pg_gist_hamming/pg_gist_hamming--1.0--1.1.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "ALTER EXTENSION pg_gist_hamming UPDATE TO '1.1'" to load this file. \quit

-- Index-only scan support new in 9.5.

CREATE FUNCTION gbt_int8_fetch(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

ALTER OPERATOR FAMILY gist_int8_ops USING gist ADD
	FUNCTION	9 (int8, int8) gbt_int8_fetch (internal) ;
