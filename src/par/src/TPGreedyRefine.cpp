///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////
#include "TPRefiner.h"
#include "TPHypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

// ------------------------------------------------------------------------------
// K-way hyperedge greedy refinement
// ------------------------------------------------------------------------------

namespace par {

// Implement the greedy refinement pass
// Different from the FM refinement, greedy refinement
// only accepts possible gain
float TPgreedyRefine::Pass(const HGraph hgraph,
                           const matrix<float>& max_block_balance,
                           matrix<float>& block_balance, // the current block balance
                           matrix<int>& net_degs, // the current net degree
                           std::vector<float>& cur_paths_cost, // the current path cost
                           TP_partition& solution,
                           std::vector<bool>& visited_vertices_flag) const 
{
  float total_gain = 0.0; // total gain improvement
  int num_move = 0;
  for (int hyperedge_id = 0; hyperedge_id < hgraph->num_hyperedges_; 
       hyperedge_id++) {
    // check if the hyperedge is a straddled_hyperedge    
    std::set<int> block_set; 
    for (int block_id = 0; block_id < num_parts_; block_id++) {
      if (net_degs[hyperedge_id][block_id] > 0) {
        block_set.insert(block_id);
      }
    }
    // ignore the hyperedge if it's fully within one block
    if (block_set.size() <= 1) {
      continue;
    }
    // updated the iteration
    num_move++;
    if (num_move >= max_moves_) {
      return total_gain;
    }  
    // find the best candidate block
    // the initialization of best_gain is 0.0
    // define a lambda function to compare two TP_gain_hyperedge (>=)
    auto CompareGainHyperedge = [&](const TP_gain_hyperedge& a, const TP_gain_hyperedge& b) {
      if (a->GetGain() > b->GetGain()) {
        return true;
      } else if (a->GetGain() == b->GetGain() && 
        hgraph->GetHyperedgeVerWtSum(a->GetHyperedge()) 
          <  hgraph->GetHyperedgeVerWtSum(b->GetHyperedge()) {
        return true; // break ties based on vertex weight summation of the hyperedge
      } else {
        return false;
      }
    };
    
    int best_candidate_block = -1;
    TP_gain_hyperedge best_gain_hyperedge = std::make_shared<HyperedgeGain>();
    best_gain_hyperedge->SetGain(0.0f); // we only accept positive gain
    for (int to_pid = 0; to_pid < num_parts_; to_pid++) {
      if (CheckHyperedgeMoveLegality(hyperedge_id, to_pid, hgraph, solution,
          block_balance, max_block_balance) == true) {
        TP_gain_hyperedge gain_hyperedge = CalculateHyperedgeGain(hyperedge_id, 
            to_pid, hgraph, solution, cur_paths_cost, net_degs);
        if (CompareGainHyperedge(gain_hyperedge, best_gain_hyperedge) == true) {
          best_candidate_block = to_pid;
          best_gain_hyperedge = gain_hyperedge;
        }        
      }
    }

    if (best_candidate_block > -1) {
      AcceptHyperedgeMove(best_gain_hyperedge,
                          hgraph, 
                          total_gain, 
                          solution,
                          cur_paths_cost,
                          block_balance,
                          net_degs); 
    }
  }

  // finish traversing all the hyperedges
  return total_gain;
}

}  // namespace par