triton_part_hypergraph -hypergraph_file sparcT1_core.hgr  \
-num_parts 3 -balance_constraint 2 -thr_coarsen_hyperedge_size_skip 50 \
-max_num_vcycle 4 -max_moves 100 -seed 10 -num_initial_solutions 100 \
-coarsening_ratio 1.5 -max_coarsen_iters 30
exit
