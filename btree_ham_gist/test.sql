
CREATE OR REPLACE FUNCTION get_random_number(BIGINT) RETURNS BIGINT AS $$
DECLARE
	max_val ALIAS FOR $1;
BEGIN
	RETURN trunc((random() * (max_val::numeric))::numeric);
END;
$$ LANGUAGE 'plpgsql' STRICT;

DROP TABLE IF EXISTS testing;
CREATE TEMP TABLE testing(val int8);



INSERT INTO
  testing (val)
SELECT
  get_random_number(x'0000FFFFFFFFFFFF'::int8)
FROM
  generate_series(1,100000) i;

CREATE INDEX test_index ON testing USING gist (val);

