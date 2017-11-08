-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION bktree" to load this file. \quit

CREATE TYPE bktree_area AS
(
	center   int8,
	distance int8
);

CREATE OR REPLACE FUNCTION bktree_area_match(int8, bktree_area) RETURNS boolean AS
'MODULE_PATHNAME','bktree_area_match'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION bktree_eq_match(int8, int8) RETURNS boolean AS
'MODULE_PATHNAME','bktree_eq_match'
LANGUAGE C IMMUTABLE STRICT;


CREATE OPERATOR <@ (
	LEFTARG = int8,
	RIGHTARG = bktree_area,
	PROCEDURE = bktree_area_match,
	RESTRICT = contsel,
	JOIN = contjoinsel);

CREATE OPERATOR = (
	LEFTARG = int8,
	RIGHTARG = int8,
	PROCEDURE = bktree_eq_match,
	RESTRICT = contsel,
	JOIN = contjoinsel);

CREATE OR REPLACE FUNCTION bktree_config(internal, internal) RETURNS void AS
'MODULE_PATHNAME','bktree_config'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION bktree_choose(internal, internal) RETURNS void AS
'MODULE_PATHNAME','bktree_choose'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION bktree_inner_consistent(internal, internal) RETURNS void AS
'MODULE_PATHNAME','bktree_inner_consistent'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION bktree_leaf_consistent(internal, internal) RETURNS boolean AS
'MODULE_PATHNAME','bktree_leaf_consistent'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION bktree_picksplit(internal, internal) RETURNS void AS
'MODULE_PATHNAME','bktree_picksplit'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR CLASS bktree_ops
	 FOR TYPE int8 USING spgist AS
	 OPERATOR 1  <@ (int8, bktree_area),
	 OPERATOR 2  =  (int8, int8),
	 FUNCTION 1  bktree_config(internal, internal),
	 FUNCTION 2  bktree_choose(internal, internal),
	 FUNCTION 3  bktree_picksplit(internal, internal),
	 FUNCTION 4  bktree_inner_consistent(internal, internal),
	 FUNCTION 5  bktree_leaf_consistent(internal, internal);

-- Utility functions


CREATE OR REPLACE FUNCTION int64_to_bitstring(int8) RETURNS cstring AS
'MODULE_PATHNAME','int64_to_bitstring'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION bitstring_to_int64(cstring) RETURNS int8 AS
'MODULE_PATHNAME','bitstring_to_int64'
LANGUAGE C IMMUTABLE STRICT;


-- Convenience operator for accessing the distance mechanism.

CREATE OR REPLACE FUNCTION bktree_get_distance(int8, int8) RETURNS int8 AS
'MODULE_PATHNAME','bktree_get_distance'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR <-> (
	LEFTARG = int8,
	RIGHTARG = int8,
	PROCEDURE = bktree_get_distance
	);
