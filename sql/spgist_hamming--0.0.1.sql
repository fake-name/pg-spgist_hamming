/*
 * Author: The maintainer's name
 * Created at: 2014-07-27 01:22:45 -0700
 *
 */

--
-- This is a example code genereted automaticaly
-- by pgxn-utils.

-- This is how you define a C function in PostgreSQL.
CREATE OR REPLACE FUNCTION spgist_hamming(text)
RETURNS text
AS 'spgist_hamming'
LANGUAGE C IMMUTABLE STRICT;

-- See more: http://www.postgresql.org/docs/current/static/xfunc-c.html
