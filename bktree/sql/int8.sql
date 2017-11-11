-- int8 check

-- SELECT pg_sleep(20);

CREATE TABLE int8tmp_2 (a int8);

\copy int8tmp_2 from 'data/int8.data'

SET enable_seqscan=on;

SELECT count(*) FROM int8tmp_2 WHERE a <  464571291354841;

SELECT count(*) FROM int8tmp_2 WHERE a <= 464571291354841;

SELECT count(*) FROM int8tmp_2 WHERE a  = 464571291354841;

SELECT count(*) FROM int8tmp_2 WHERE a >= 464571291354841;

SELECT count(*) FROM int8tmp_2 WHERE a >  464571291354841;

SELECT a, a <-> 464571291354841 FROM int8tmp_2 WHERE a <@ (464571291354841, 4);

EXPLAIN (COSTS OFF)
SELECT a, a <-> 464571291354841 FROM int8tmp_2 WHERE a <@ (464571291354841, 4);

\copy int8tmp_2 from 'data/incr.data'

CREATE INDEX int8idx_2 ON int8tmp_2 USING spgist ( a bktree_ops );

-- \copy int8tmp_2 from 'data/test_data_1.data'
-- \copy int8tmp_2 from 'data/int8.data'

SET enable_seqscan=off;

SELECT count(*) FROM int8tmp_2 WHERE a <  464571291354841::int8;
SELECT count(*) FROM int8tmp_2 WHERE a <= 464571291354841::int8;
SELECT count(*) FROM int8tmp_2 WHERE a >= 464571291354841::int8;
SELECT count(*) FROM int8tmp_2 WHERE a >  464571291354841::int8;

SELECT count(*) FROM int8tmp_2 WHERE a  = 464571291354841::int8;

EXPLAIN (COSTS OFF)
SELECT a, a <-> 464571291354841, a <@ (464571291354841, 4), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ (464571291354841, 4);
EXPLAIN (COSTS OFF)
SELECT a, a <-> 464571291354841, a <@ (464571291354841, 4), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ (464571291354841, 4) ORDER BY a <-> 464571291354841 ASC;

SELECT a, a <->  10, a <@ ( 10, 4), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ ( 10, 4) ORDER BY a <->  10 ASC, a ASC;
SELECT a, a <-> 100, a <@ (100, 4), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ (100, 4) ORDER BY a <-> 100 ASC, a ASC;
SELECT a, a <->   2, a <@ (  2, 4), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ (  2, 4) ORDER BY a <->   2 ASC, a ASC;

SELECT a, a <->  10, a <@ ( 10, 1), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ ( 10, 1) ORDER BY a <->  10 ASC, a ASC;
SELECT a, a <-> 100, a <@ (100, 1), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ (100, 1) ORDER BY a <-> 100 ASC, a ASC;
SELECT a, a <->   2, a <@ (  2, 1), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ (  2, 1) ORDER BY a <->   2 ASC, a ASC;

SELECT a, a <->  10, a <@ ( 10, 2), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ ( 10, 2) ORDER BY a <->  10 ASC, a ASC;
SELECT a, a <-> 100, a <@ (100, 2), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ (100, 2) ORDER BY a <-> 100 ASC, a ASC;
SELECT a, a <->   2, a <@ (  2, 2), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ (  2, 2) ORDER BY a <->   2 ASC, a ASC;

EXPLAIN (COSTS OFF)
SELECT a, a <->  10, a <@ ( 10, 1), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ ( 10, 1) ORDER BY a <->  10 ASC, a ASC;
EXPLAIN (COSTS OFF)
SELECT a, a <-> 100, a <@ (100, 1), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ (100, 1) ORDER BY a <-> 100 ASC, a ASC;
EXPLAIN (COSTS OFF)
SELECT a, a <->   2, a <@ (  2, 1), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ (  2, 1) ORDER BY a <->   2 ASC, a ASC;

EXPLAIN (COSTS OFF)
SELECT a, a <->  10, a <@ ( 10, 2), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ ( 10, 2) ORDER BY a <->  10 ASC, a ASC;
EXPLAIN (COSTS OFF)
SELECT a, a <-> 100, a <@ (100, 2), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ (100, 2) ORDER BY a <-> 100 ASC, a ASC;
EXPLAIN (COSTS OFF)
SELECT a, a <->   2, a <@ (  2, 2), int64_to_bitstring(a) FROM int8tmp_2 WHERE a <@ (  2, 2) ORDER BY a <->   2 ASC, a ASC;
