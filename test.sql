
DROP TABLE IF EXISTS testing;


DROP EXTENSION btree_ham_gist;
CREATE EXTENSION btree_ham_gist;

CREATE TEMP TABLE testing(val int8);



CREATE OR REPLACE FUNCTION get_random_number(BIGINT) RETURNS BIGINT AS $$
DECLARE
	max_val ALIAS FOR $1;
BEGIN
	RETURN trunc((random() * (max_val::numeric))::numeric);
END;
$$ LANGUAGE 'plpgsql' STRICT;


INSERT INTO
  testing (val)
SELECT
  get_random_number(x'0000FFFFFFFFFFFF'::int8)
FROM
  generate_series(1,1000) i;

CREATE INDEX test_index ON testing USING gist (val gist_int8_ops);

