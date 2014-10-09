Btree_ham_gist
==============

This will, with any luck, eventually be a GiST index implementation that allows
fast querying for items by hamming distance.

The general idea is to use a BK-tree structure implemented using the B-tree facilities in Postgres.
With any luck, it *should* be possible to implement without deep understanding of PostgreSQL,
which is not something I have, currently.

This is intended to allow fast fuzzy searches on phash datasets, as is needed for things like
approximate-image-searches, as well as other phash-driven systems (AcousID, for example).

I also have *NO* idea what I'm doing.


