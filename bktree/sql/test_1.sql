-- int8 check

-- SELECT pg_sleep(20);

CREATE TABLE test1_tmp_table (a int8);

\copy test1_tmp_table from 'data/test_data_1.data'

CREATE INDEX test1_tmp_table_idx ON test1_tmp_table USING spgist ( a bktree_ops );

SET enable_seqscan=off;

-- These should work (I'm not sure how)
SELECT count(*) FROM test1_tmp_table WHERE a  = 464571291354841::int8;
SELECT count(*) FROM test1_tmp_table WHERE a  = 7::int8;

EXPLAIN (COSTS OFF) SELECT count(*) FROM test1_tmp_table WHERE a  = 7::int8;

-- These should fail, since we don't provide operators
-- for the relevant conditionals (<, >, <=, >=)
-- Apparently the SP-GiST system just coerces the
-- result to an empty set if it doesn't know how
-- to apply the operator
SELECT count(*) FROM test1_tmp_table WHERE a  < 7::int8;
SELECT count(*) FROM test1_tmp_table WHERE a  > 7::int8;
SELECT count(*) FROM test1_tmp_table WHERE a  <= 7::int8;
SELECT count(*) FROM test1_tmp_table WHERE a  >= 7::int8;


-- Search value
SELECT int64_to_bitstring(5);
-- Now the actual searches.
SELECT a, a <@ (5, 1), a <-> 5, int64_to_bitstring(a) FROM test1_tmp_table WHERE  a <@ (5, 1);
SELECT a, a <@ (5, 2), a <-> 5, int64_to_bitstring(a) FROM test1_tmp_table WHERE  a <@ (5, 2);
SELECT a, a <@ (5, 2), a <-> 5, int64_to_bitstring(a) FROM test1_tmp_table WHERE  a <@ (5, 3);
SELECT a, a <@ (5, 4), a <-> 5, int64_to_bitstring(a) FROM test1_tmp_table WHERE  a <@ (5, 4);


EXPLAIN (COSTS OFF)
SELECT a, a <@ (5, 1), a <-> 5, int64_to_bitstring(a) FROM test1_tmp_table WHERE  a <@ (5, 1);
EXPLAIN (COSTS OFF)
SELECT a, a <@ (5, 2), a <-> 5, int64_to_bitstring(a) FROM test1_tmp_table WHERE  a <@ (5, 2);
EXPLAIN (COSTS OFF)
SELECT a, a <@ (5, 2), a <-> 5, int64_to_bitstring(a) FROM test1_tmp_table WHERE  a <@ (5, 3);
EXPLAIN (COSTS OFF)
SELECT a, a <@ (5, 4), a <-> 5, int64_to_bitstring(a) FROM test1_tmp_table WHERE  a <@ (5, 4);
