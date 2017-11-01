-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION vptree" to load this file. \quit

CREATE TYPE vptree_area AS
(
    center	 _int4,
    distance float8
);

CREATE OR REPLACE FUNCTION vptree_area_match(_int4, vptree_area) RETURNS boolean AS
'MODULE_PATHNAME','vptree_area_match'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR <@ (
	LEFTARG = _int4,
	RIGHTARG = vptree_area, 
	PROCEDURE = vptree_area_match,
	RESTRICT = contsel,
	JOIN = contjoinsel);

CREATE OR REPLACE FUNCTION vptree_config(internal, internal) RETURNS void AS
'MODULE_PATHNAME','vptree_config'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION vptree_choose(internal, internal) RETURNS void AS
'MODULE_PATHNAME','vptree_choose'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION vptree_inner_consistent(internal, internal) RETURNS boolean AS
'MODULE_PATHNAME','vptree_inner_consistent'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION vptree_leaf_consistent(internal, internal) RETURNS void AS
'MODULE_PATHNAME','vptree_leaf_consistent'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION vptree_picksplit(internal, internal) RETURNS void AS
'MODULE_PATHNAME','vptree_picksplit'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR CLASS vptree_ops
   FOR TYPE _int4 USING spgist AS
   OPERATOR 1  <@ (_int4, vptree_area),
   FUNCTION 1  vptree_config(internal, internal),
   FUNCTION 2  vptree_choose(internal, internal),
   FUNCTION 3  vptree_picksplit(internal, internal),
   FUNCTION 4  vptree_inner_consistent(internal, internal),
   FUNCTION 5  vptree_leaf_consistent(internal, internal);

