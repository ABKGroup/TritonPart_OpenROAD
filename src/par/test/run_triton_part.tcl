triton_part_hypergraph -hypergraph_file sparcT1_core.hgr \
-num_parts 4 -balance_constraint 2 -thr_coarsen_hyperedge_size_skip 100 -max_num_vcycle 1 -num_initial_solutions 100 \
-num_coarsen_solutions 4 \
-seed 1
exit
