triton_part_hypergraph -hypergraph_file sparcT1_core.hgr \
-num_parts 4 -balance_constraint 2 -thr_coarsen_hyperedge_size_skip 1000 \
-min_num_vertices_each_part 4 \
-num_initial_solutions 50 \
-seed 0
exit
