spgist_hamming
==============

This will, with any luck, eventually be a sp-GiST index implementation that allows 
fast querying for items by hamming distance.

The general idea is to use a VP-tree structure, which can share a lot of the underlying 
implementation details with a k-d tree, for which there is already a sp-GiST 
implementation. It *should* be possible to implement without deep understanding of 
PostgreSQL, which is not something I have, currently.

I also have *NO* idea what I'm doing.


To build it, just do this:

    make
    make installcheck
    make install

If you encounter an error such as:

    "Makefile", line 8: Need an operator

You need to use GNU make, which may well be installed on your system as
`gmake`:

    gmake
    gmake install
    gmake installcheck

If you encounter an error such as:

    make: pg_config: Command not found

Be sure that you have `pg_config` installed and in your path. If you used a
package management system such as RPM to install PostgreSQL, be sure that the
`-devel` package is also installed. If necessary tell the build process where
to find it:

    env PG_CONFIG=/path/to/pg_config make && make installcheck && make install

And finally, if all that fails (and if you're on PostgreSQL 8.1 or lower, it
likely will), copy the entire distribution directory to the `contrib/`
subdirectory of the PostgreSQL source tree and try it there without
`pg_config`:

    env NO_PGXS=1 make && make installcheck && make install

If you encounter an error such as:

    ERROR:  must be owner of database regression

You need to run the test suite using a super user, such as the default
"postgres" super user:

    make installcheck PGUSER=postgres

Once spgist_hamming is installed, you can add it to a database. If you're running
PostgreSQL 9.1.0 or greater, it's a simple as connecting to a database as a
super user and running:

    CREATE EXTENSION spgist_hamming;

If you've upgraded your cluster to PostgreSQL 9.1 and already had spgist_hamming
installed, you can upgrade it to a properly packaged extension with:

    CREATE EXTENSION spgist_hamming FROM unpackaged;

For versions of PostgreSQL less than 9.1.0, you'll need to run the
installation script:

    psql -d mydb -f /path/to/pgsql/share/contrib/spgist_hamming.sql

If you want to install spgist_hamming and all of its supporting objects into a specific
schema, use the `PGOPTIONS` environment variable to specify the schema, like
so:

    PGOPTIONS=--search_path=extensions psql -d mydb -f spgist_hamming.sql

Dependencies
------------
The `spgist_hamming` data type has no dependencies other than PostgreSQL.

Copyright and License
---------------------

Copyright (c) 2014 Fake Name.

