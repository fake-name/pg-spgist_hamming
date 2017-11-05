-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION vptree" to load this file. \quit

CREATE TYPE vptree_area AS
(
	center   int8,
	distance float8
);

CREATE OR REPLACE FUNCTION vptree_area_match(int8, vptree_area) RETURNS boolean AS
'MODULE_PATHNAME','vptree_area_match'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION vptree_eq_match(int8, int8) RETURNS boolean AS
'MODULE_PATHNAME','vptree_eq_match'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION vptree_get_distance(int8, int8) RETURNS float8 AS
'MODULE_PATHNAME','vptree_get_distance'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR <@ (
	LEFTARG = int8,
	RIGHTARG = vptree_area,
	PROCEDURE = vptree_area_match,
	RESTRICT = contsel,
	JOIN = contjoinsel);

CREATE OPERATOR = (
	LEFTARG = int8,
	RIGHTARG = int8,
	PROCEDURE = vptree_eq_match,
	RESTRICT = contsel,
	JOIN = contjoinsel);

-- Utility operator for accessing the distance mechanism.
CREATE OPERATOR <-> (
	LEFTARG = int8,
	RIGHTARG = int8,
	PROCEDURE = vptree_get_distance
	);

CREATE OR REPLACE FUNCTION vptree_config(internal, internal) RETURNS void AS
'MODULE_PATHNAME','vptree_config'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION vptree_choose(internal, internal) RETURNS void AS
'MODULE_PATHNAME','vptree_choose'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION vptree_inner_consistent(internal, internal) RETURNS void AS
'MODULE_PATHNAME','vptree_inner_consistent'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION vptree_leaf_consistent(internal, internal) RETURNS boolean AS
'MODULE_PATHNAME','vptree_leaf_consistent'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION vptree_picksplit(internal, internal) RETURNS void AS
'MODULE_PATHNAME','vptree_picksplit'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR CLASS vptree_ops
	 FOR TYPE int8 USING spgist AS
	 OPERATOR 1  <@ (int8, vptree_area),
	 OPERATOR 2  =   (int8, int8),
	 FUNCTION 1  vptree_config(internal, internal),
	 FUNCTION 2  vptree_choose(internal, internal),
	 FUNCTION 3  vptree_picksplit(internal, internal),
	 FUNCTION 4  vptree_inner_consistent(internal, internal),
	 FUNCTION 5  vptree_leaf_consistent(internal, internal);

