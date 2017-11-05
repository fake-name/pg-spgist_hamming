-- int8 check

-- SELECT pg_sleep(20);

CREATE TABLE int8tmp_2 (a int8);

\copy int8tmp_2 from 'data/incr.data'

SET enable_seqscan=on;

SELECT count(*) FROM int8tmp_2 WHERE a <  464571291354841;

SELECT count(*) FROM int8tmp_2 WHERE a <= 464571291354841;

SELECT count(*) FROM int8tmp_2 WHERE a  = 464571291354841;

SELECT count(*) FROM int8tmp_2 WHERE a >= 464571291354841;

SELECT count(*) FROM int8tmp_2 WHERE a >  464571291354841;

SELECT a, a <@ '464571291354841' FROM int8tmp_2 ORDER BY a <@ '464571291354841' LIMIT 3;

CREATE INDEX int8idx_2 ON int8tmp_2 USING spgist ( a vptree_ops );

-- \copy int8tmp_2 from 'data/test_data_1.data'
-- \copy int8tmp_2 from 'data/int8.data'

SET enable_seqscan=off;

SELECT count(*) FROM int8tmp_2 WHERE a <  464571291354841::int8;
SELECT count(*) FROM int8tmp_2 WHERE a <= 464571291354841::int8;
SELECT count(*) FROM int8tmp_2 WHERE a >= 464571291354841::int8;
SELECT count(*) FROM int8tmp_2 WHERE a >  464571291354841::int8;

SELECT count(*) FROM int8tmp_2 WHERE a  = 464571291354841::int8;

EXPLAIN (COSTS OFF)
SELECT a, a <@ '464571291354841' FROM int8tmp_2 ORDER BY a <@ '464571291354841' LIMIT 3;
SELECT a, a <@ '464571291354841' FROM int8tmp_2 ORDER BY a <@ '464571291354841' LIMIT 3;
