-- int8 check

-- SELECT pg_sleep(20);

CREATE TABLE int8tmp_2 (a int8);

\copy int8tmp_2 from 'data/int8.data'
\copy int8tmp_2 from 'data/incr.data'

SET enable_seqscan=on;

SELECT count(*) FROM int8tmp_2 WHERE a <  464571291354841;

SELECT count(*) FROM int8tmp_2 WHERE a <= 464571291354841;

SELECT count(*) FROM int8tmp_2 WHERE a  = 464571291354841;

SELECT count(*) FROM int8tmp_2 WHERE a >= 464571291354841;

SELECT count(*) FROM int8tmp_2 WHERE a >  464571291354841;

-- These succeed, since the system falls back
-- to a sequential scan because of the lack of support
-- for these operators in the index
SELECT count(*) FROM int8tmp WHERE a  < 7::int8;
SELECT count(*) FROM int8tmp WHERE a  > 7::int8;
SELECT count(*) FROM int8tmp WHERE a  <= 7::int8;
SELECT count(*) FROM int8tmp WHERE a  >= 7::int8;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM int8tmp WHERE a  < 7::int8;

SELECT a, a <@ ('5', 1), a <-> 5 FROM int8tmp WHERE  a <@ ('5', 1);
SELECT a, a <@ ('5', 2), a <-> 5 FROM int8tmp WHERE  a <@ ('5', 2);
SELECT a, a <@ ('5', 2), a <-> 5 FROM int8tmp WHERE  a <@ ('5', 3);
SELECT a, a <@ ('5', 4), a <-> 5 FROM int8tmp WHERE  a <@ ('5', 4);

SELECT a, a <@ (5, 1), a <-> 5 FROM int8tmp WHERE  a <@ (5, 1);
SELECT a, a <@ (5, 2), a <-> 5 FROM int8tmp WHERE  a <@ (5, 2);
SELECT a, a <@ (5, 2), a <-> 5 FROM int8tmp WHERE  a <@ (5, 3);
SELECT a, a <@ (5, 4), a <-> 5 FROM int8tmp WHERE  a <@ (5, 4);


CREATE INDEX int8idx_2 ON int8tmp_2 USING spgist ( a vptree_ops );

SET enable_seqscan=off;

-- These should fail, since we don't provide operators
-- for the relevant conditionals (<, >, <=, >=),
-- and sequential-scans are disabled.
-- Apparently the SP-GiST system just coerces the
-- result to an empty set if it doesn't know how
-- to apply the operator
SELECT count(*) FROM int8tmp WHERE a  < 7::int8;
SELECT count(*) FROM int8tmp WHERE a  > 7::int8;
SELECT count(*) FROM int8tmp WHERE a  <= 7::int8;
SELECT count(*) FROM int8tmp WHERE a  >= 7::int8;

SELECT a, a <@ ('5', 1), a <-> 5 FROM int8tmp WHERE  a <@ ('5', 1);
SELECT a, a <@ ('5', 2), a <-> 5 FROM int8tmp WHERE  a <@ ('5', 2);
SELECT a, a <@ ('5', 2), a <-> 5 FROM int8tmp WHERE  a <@ ('5', 3);
SELECT a, a <@ ('5', 4), a <-> 5 FROM int8tmp WHERE  a <@ ('5', 4);

SELECT a, a <@ (5, 1), a <-> 5 FROM int8tmp WHERE  a <@ (5, 1);
SELECT a, a <@ (5, 2), a <-> 5 FROM int8tmp WHERE  a <@ (5, 2);
SELECT a, a <@ (5, 2), a <-> 5 FROM int8tmp WHERE  a <@ (5, 3);
SELECT a, a <@ (5, 4), a <-> 5 FROM int8tmp WHERE  a <@ (5, 4);

SELECT a, a <@ (93382613193471632, 1000000), a <-> 93382613193471632 FROM int8tmp_2 WHERE a <@ (93382613193471632, 1000000) ORDER BY a <-> 93382613193471632 LIMIT 3;

SELECT a, a <@ (5, 4), a <-> 5 FROM int8tmp_2 WHERE a <@ (5, 4) ORDER BY a <-> 5 LIMIT 3;

EXPLAIN (COSTS OFF)
SELECT a, a <@ (64571291354841, 100), a <-> 64571291354841 FROM int8tmp_2 WHERE a <@ (64571291354841, 100) ORDER BY a <-> 64571291354841 LIMIT 3;

EXPLAIN (COSTS OFF)
SELECT a, a <@ (464571291354841, 100) FROM int8tmp_2 WHERE a <@ (464571291354841, 100);
