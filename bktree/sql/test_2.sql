-- int8 check

-- SELECT pg_sleep(20);

CREATE TABLE test2_tmp_table (a int8);

\copy test2_tmp_table from 'data/incr.data'

EXPLAIN (COSTS OFF) SELECT count(*) FROM test2_tmp_table WHERE a  = 7::int8;

CREATE INDEX test2_tmp_table_idx ON test2_tmp_table USING spgist ( a bktree_ops );

SET enable_seqscan=off;

-- These should work
SELECT count(*) FROM test2_tmp_table WHERE a  = 464571291354841::int8;
SELECT count(*) FROM test2_tmp_table WHERE a  = 7::int8;

EXPLAIN (COSTS OFF) SELECT count(*) FROM test2_tmp_table WHERE a  = 7::int8;

-- These should fail, since we don't provide operators
-- for the relevant conditionals (<, >, <=, >=)
-- Apparently the SP-GiST system just coerces the
-- result to an empty set if it doesn't know how
-- to apply the operator
SELECT count(*) FROM test2_tmp_table WHERE a  < 7::int8;
SELECT count(*) FROM test2_tmp_table WHERE a  > 7::int8;
SELECT count(*) FROM test2_tmp_table WHERE a  <= 7::int8;
SELECT count(*) FROM test2_tmp_table WHERE a  >= 7::int8;

-- Now the actual searches.
SELECT a, a <@ (0, 1), a <-> 0, int64_to_bitstring(a) FROM test2_tmp_table WHERE  a <@ (0, 1) ORDER BY a;
SELECT a, a <@ (0, 2), a <-> 0, int64_to_bitstring(a) FROM test2_tmp_table WHERE  a <@ (0, 2) ORDER BY a;
SELECT a, a <@ (0, 2), a <-> 0, int64_to_bitstring(a) FROM test2_tmp_table WHERE  a <@ (0, 3) ORDER BY a;
SELECT a, a <@ (0, 4), a <-> 0, int64_to_bitstring(a) FROM test2_tmp_table WHERE  a <@ (0, 4) ORDER BY a;


-- Search value
SELECT int64_to_bitstring(1023);
-- Now the actual searches.
SELECT a, a <@ (1023, 1), a <-> 1023, int64_to_bitstring(a) FROM test2_tmp_table WHERE  a <@ (1023, 1) ORDER BY a;
SELECT a, a <@ (1023, 2), a <-> 1023, int64_to_bitstring(a) FROM test2_tmp_table WHERE  a <@ (1023, 2) ORDER BY a;
SELECT a, a <@ (1023, 2), a <-> 1023, int64_to_bitstring(a) FROM test2_tmp_table WHERE  a <@ (1023, 3) ORDER BY a;
SELECT a, a <@ (1023, 4), a <-> 1023, int64_to_bitstring(a) FROM test2_tmp_table WHERE  a <@ (1023, 4) ORDER BY a;


EXPLAIN (COSTS OFF)
SELECT a, a <@ (5, 1), a <-> 5, int64_to_bitstring(a) FROM test2_tmp_table WHERE  a <@ (5, 1) ORDER BY a;
EXPLAIN (COSTS OFF)
SELECT a, a <@ (5, 2), a <-> 5, int64_to_bitstring(a) FROM test2_tmp_table WHERE  a <@ (5, 2) ORDER BY a;
EXPLAIN (COSTS OFF)
SELECT a, a <@ (5, 2), a <-> 5, int64_to_bitstring(a) FROM test2_tmp_table WHERE  a <@ (5, 3) ORDER BY a;
EXPLAIN (COSTS OFF)
SELECT a, a <@ (5, 4), a <-> 5, int64_to_bitstring(a) FROM test2_tmp_table WHERE  a <@ (5, 4) ORDER BY a;
