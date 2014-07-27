spgist_hamming
==============

This will, with any luck, eventually be a sp-GiST index implementation that allows 
fast querying for items by hamming distance.

The general idea is to use a VP-tree structure, which can share a lot of the underlying 
implementation details with a k-d tree, for which there is already a sp-GiST 
implementation. It *should* be possible to implement without deep understanding of 
PostgreSQL, which is not something I have, currently.

This is intended to allow fast fuzzy searches on phash datasets, as is needed for things like 
approximate-image-searches, as well as other phash-driven systems (AcousID, for example).

I also have *NO* idea what I'm doing.


