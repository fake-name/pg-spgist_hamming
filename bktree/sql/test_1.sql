-- int8 check

-- SELECT pg_sleep(20);

CREATE TABLE int8tmp (a int8);

\copy int8tmp from 'data/test_data_1.data'

CREATE INDEX int8idx ON int8tmp USING spgist ( a vptree_ops );

SET enable_seqscan=off;

-- These should work (I'm not sure how)
SELECT count(*) FROM int8tmp WHERE a  = 464571291354841::int8;
SELECT count(*) FROM int8tmp WHERE a  = 7::int8;

EXPLAIN (COSTS OFF) SELECT count(*) FROM int8tmp WHERE a  = 7::int8;

-- These should fail, since we don't provide operators
-- for the relevant conditionals (<, >, <=, >=)
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

-- EXPLAIN (COSTS OFF)
