pg-spgist_hamming
==============

Tested on PostgreSQL 9.6 and 11 

This repository contains two SP-GiST index extensions.

Principally, they detail my progress while implementing a [BK-Tree][1] as a native
C PostgreSQL indexing extension. This is extremely useful for certain types of 
searches, primarily related to fuzzy-image searching.

The subdirectories are as follows:

 - vptree:
    This is a simple Viewpoint-Tree index using the SP-GiST indexing facilities. 
    Originally, this was written by Alexander Korotkov (a.korotkov@postgrespro.ru),
    though that implementation was for an earlier version of the SP_GiST API. The 
    version in this repository has been updated for modern (9.6, 10) postgresql, 
    and was the starting point for my own index implementation.

    Without this file, I wouldn't have been able to get anywhere, having a functional
    basis to work from was enormously beneficial, and I'm extremely grateful to Alexander
    (on the pgsql-hackers mailing list) for the starting point.

 - bktree:
     This is my index implementation. It implements a BK-tree index across a 64-bit
     p-hash data type. Basically, it results in a 65-ary tree, where the child
     nodes are distributed by the edit-distance from the intermediate node.

     This is (basically) a re-implementation of my pure-C++ BK-tree implementation
     that is located [here][2]. There are minor changes due to the requirements 
     of the SP-GiST system (normally, this would be a 64-ary tree, but because
     SP-GiST inner tuples cannot store data, you need a additional branch for 
     the matching case (e.g. edit distance of 0), whereas my C++ implementation 
     can store the matching data directly in the inner tuple, so the matching 
     case does not need an additional branch).

     As a side-benefit of having a robust and well-tested C++ implementation 
     of a BK-tree, I can make stronger garantees about the correctness of the 
     SP-GiST index implementation, simply by porting the tests for the C++ 
     version to use the PG version. That was done, and the resulting tests
     are [here][3] (see all the `Test_db_BKTree*.py` files).

 - old:
    Initially, I wasn't sure if I needed a SP-GiST or a plain GiST index, so 
    the contents of this directory were experimenting with plain GiST indexes.
    It didn't really go anywhere, but it's vaguely interesting. I may revisit
    the GiST implementation at some point.


Anyways, in benchmarking, the PostgreSQL implementation of a BK-tree is approximately 
33% - 50% as fast as the C++ implementation, presumably due to the additional 
overhead of the PG tuple, SP-GiST and GiST mechanisms. While this is annoying, it's 
not a significant-enough performance loss to motivate me to continue using 
the C++ version, due to the significant implementation complexity of maintaining 
an out-of-database additional index. Adding more RAM to the PostgreSQL host may also help
here. My test system has 32 GB of ram, and the C++ BK-Tree implementation alone requires 
~18 GB to contain the entire dataset.


[1] : http://blog.notdot.net/2007/4/Damn-Cool-Algorithms-Part-1-BK-Trees  
[2] : https://github.com/fake-name/IntraArchiveDeduplicator/blob/92da07a75928b803a23d0e2940c40013da8ea115/deduplicator/bktree.hpp  
[3] : https://github.com/fake-name/IntraArchiveDeduplicator/tree/master/Tests  

------

Quickstart:

This module has a simple makefile that uses `pg_config` to do it's magic. Check you have the `pg_config` shell command, and that it's output looks reasonable.

If you do, installing is a two steps:

```
cd bktree
make
sudo make install
sudo make installcheck   # to run tests that check everything installed correctly.
```

Note that installcheck currently fails on postgresql not 9.5, due to minor changes in the output of `EXPLAIN ANALYZE`. The extension works correctly, but the tests work by `diff`ing the output of queries executed via `psql`, so minor changes in the output formatting can produce false breakages.



Once you have it installed:


```
# Enable extension in current database (Note: This is per-database, so if you want to use it on 
# multiple DBs, you'll have to enable it in each.
CREATE EXTENSION bktree;

# Use the enabled extension to create an index. 
# phash_column MUST be a int64 ("bigint") type.
CREATE INDEX bk_index_name ON table_name USING spgist (phash_column bktree_ops);

# Query across the table within a specified edit distance.
SELECT <columns here> FROM table_name WHERE phash_column <@ (target_phash_int64, search_distance_int);
```

You'll need to replace things like `bk_index_name`, `table_name`, `target_phash_int64`, `search_distance_int`, 
and `phash_column` with appropriate values for your database.

`phash_column` must be a column of type `bigint`. Currently, only 64-bit phash values are supported, and they're 
stored in signed format. 
