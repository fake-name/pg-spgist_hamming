\set ECHO 0
BEGIN;
\i sql/spgist_hamming.sql
\set ECHO all

-- You should write your tests

SELECT spgist_hamming('test');

ROLLBACK;
