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
#pragma once

#include <set>

#include "TPHypergraph.h"
#include "Utilities.h"
#include <tuple>
#include "utl/Logger.h"

namespace par {

// matrix is a two-dimensional vectors
template <typename T>
using matrix = std::vector<std::vector<T> >;

// TP_partition is the partitioning solution
using TP_partition = std::vector<int>; //

// TP_partition_token is the metrics of a given partition
// it consists of two part:  cost (cutsize), balance for each block
// for example, TP_partition_token.second[0] is the balance of 
// block_0
using TP_partition_token = std::pair<float, matrix<float> >;


// GoldenEvaluator
class GoldenEvaluator;
using TP_evaluator = std::shared_ptr<GoldenEvaluator>;

// ------------------------------------------------------------------------
// The implementation of GoldenEvaluator
// It's used to compute the basic properties of a partitioning solution
// ------------------------------------------------------------------------
class GoldenEvaluator {
  public:
    GoldenEvaluator() {  }

    // TODO: update the constructor
                      num_parts_, 
                                          // weight vectors
                                          e_wt_factors_,
                                          v_wt_factors_,
                                          placement_wt_factors_,
                                          // timing related weight
                                          net_timing_factor_,
                                          path_timing_factor_,
                                          path_snaking_factor_,
                                          timing_exp_factor_,
                                          extra_delay_,
                                          logger_);



    GoldenEvaluator(const int num_parts,
                    const float extra_cut_delay, 
                    const std::vector<float>& e_wt_factors, // the factor for hyperedge weight
                    const float timing_factor, // the factor for hyperedge timing weight
                    const float path_wt_factor,// weight for cutting a critical timing path
                    const float snaking_wt_factor, // snaking factor a critical timing path
                    const float timing_exp_factor, // timing exponetial factor for normalized slack
                    utl::Logger* logger)
        : num_parts_(num_parts),
          extra_cut_delay_(extra_cut_delay),
          e_wt_factors_(e_wt_factors),
          timing_factor_(timing_factor),
          path_wt_factor_(path_wt_factor),
          snaking_wt_factor_(snaking_wt_factor),
          timing_exp_factor_(timing_exp_factor),
          logger_(logger)
    {
    }
    
    GoldenEvaluator(const GoldenEvaluator&) = delete;
    GoldenEvaluator(GoldenEvaluator&) = delete;
    virtual ~GoldenEvaluator() = default;

    // calculate the vertex distribution of each net
    matrix<int> GetNetDegrees(const HGraph hgraph,
                              const TP_partition& solution) const;
                          
    // Get block balance
    matrix<float> GetBlockBalance(const HGraph hgraph,
                                  const TP_partition& solution) const;

    // calculate timing cost of a path
    float GetPathTimingScore(const HGraph hgraph, const int path_id) const;
                                 
    // calculate the cost of a path
    float CalculatePathCost(int path_id,
                            const HGraph hgraph,
                            const TP_partition& solution) const;
    
    // get the cost of all the paths: include the timing part and snaking part
    std::vector<float> GetPathsCost(const HGraph hgraph, 
                                    const TP_partition& solution) const;

    // calculate the status of timing path cuts
    // total cut, worst cut, average cut
    std::tuple<int, int, float> GetTimingCuts(const HGraph hgraph,
                                              const TP_partition& solution) const;

    // Calculate the timing cost due to the slack of hyperedge itself
    float CalculateHyperedgeTimingCost(int e, const HGraph hgraph) const;

    // Calculate the cost of a hyperedge  
    float CalculateHyperedgeCost(int e, const HGraph hgraph) const;

    // calculate the hyperedge score. score / (hyperedge.size() - 1)
    float GetNormEdgeScore(int e, const HGraph hgraph) const;
      
    // calculate the vertex weight norm
    // This is usually used to sort the vertices
    float GetVertexWeightNorm(int v, const HGraph hgraph) const;

    // calculate the placement score between vertex v and u
    float GetPlacementScore(int v, int u, const HGraph hgraph) const;

    // Get average the placement location
    float GetAvgPlacementLoc(int v, int u, const HGraph hgraph) const;

    // calculate the average placement location
    std::vector<float> GetAvgPlacementLoc(const std::vector<float>& vertex_weight_a,
                                          const std::vector<float>& vertex_weight_b,
                                          const std::vector<float>& placement_loc_a,
                                          const std::vector<float>& placement_loc_b) const;

    // calculate the hyperedges being cut
    std::vector<int> GetCutHyperedges(const HGraph hgraph, 
                                      const std::vector<int>& solution) const;


    // calculate the statistics of a given partitioning solution
    // TP_partition_token.first is the cutsize
    // TP_partition_token.second is the balance constraint
    TP_partition_token CutEvaluator(const HGraph hgraph,
                                    const std::vector<int>& solution,
                                    bool print_flag = false) const;      


    // hgraph will be updated here
    // For timing-driven flow, 
    // we need to convert the slack information to related weight
    // Basically we will transform the path_timing_attr_ to path_timing_cost_,
    // and transform hyperedge_timing_attr_ to hyperedge_timing_cost_.
    // Then overlay the path weighgts onto corresponding weights
    void InitializeTiming(HGraph hgraph) const; 

    // Update timing information of a hypergraph
    // For timing-driven flow,
    // we first need to update the timing information of all the hyperedges 
    // and paths (path_timing_attr_ and hyperedge_timing_attr_),
    // i.e., introducing extra delay on the hyperedges being cut.
    // Then we call InitializeTiming to update the corresponding weights.
    // The timing_graph_ contains all the necessary information, 
    // include the original slack for each path and hyperedge,
    // and the type of each vertex
    void UpdateTiming(HGraph hgraph, const TP_partition& solution) const;

  private:
    // user specified parameters
    const int num_parts_ = 2; // number of blocks in the partitioning
    const float extra_cut_delay_ = 1.0; // the extra delay introduced by a cut
    
    // This weight will be modifed when user call initial path
    std::vector<float> e_wt_factors_;  // the cost introduced by a cut hyperedge e is
                                       // e_wt_factors dot_product hyperedge_weights_[e]
                                       // this parameter is used by coarsening and partitioning
    const float path_wt_factor_ = 1.0; // the cost for cutting a critical timing path once. 
                                       // If a critical path is cut by 3 times,
                                       // the cost is defined as 3 * path_wt_factor_ * weight_of_path
    const float snaking_wt_factor_ = 1.0; // the cost of introducing a snaking timing path, see our paper for detailed explanation
                                          // of snaking timing paths
    
    const float timing_factor_ = 1.0;  // the factor for cutting a hyperedge with timing information
    const float timing_exp_factor_ = 2.0; // exponential factor

       
    const std::vector<float> v_wt_factors_;  // the ``weight'' of a vertex. For placement-driven coarsening,
                                             // when we merge two vertices, we need to update the location
                                             // of merged vertex based on the gravity center of these two 
                                             // vertices.  The weight of vertex v is defined
                                             // as v_wt_factors dot_product vertex_weights_[v]
                                             // this parameter is only used in coarsening
                                   
    const std::vector<float> placement_wt_factors_; // the weight for placement information. For placement-driven 
                                                    // coarsening, when we calculate the score for best-choice coarsening,
                                                    // the placement information also contributes to the score function.
                                                    // we prefer to merge two vertices which are adjacent physically
                                                    // the distance between u and v is defined as 
                                                    // norm2(placement_attr[u] - placement_attr[v], placement_wt_factors_)
                                                    // this parameter is only used during coarsening     

    bool initial_path_flag_ = false;
    const HGraph timing_graph_ = nullptr;
    utl::Logger* logger_ = nullptr;    
};

}  // namespace par