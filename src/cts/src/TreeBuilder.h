/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Clock.h"
#include "CtsOptions.h"
#include "TechChar.h"
#include "Util.h"

namespace utl {
class Logger;
}  // namespace utl

namespace cts {

class TreeBuilder
{
 public:
  TreeBuilder(CtsOptions* options,
              Clock& clk,
              TreeBuilder* parent,
              utl::Logger* logger,
              odb::dbDatabase* db = nullptr)
      : options_(options),
        clock_(clk),
        parent_(parent),
        logger_(logger),
        db_(db)
  {
    if (parent) {
      parent->children_.emplace_back(this);
    }
  }

  virtual void run() = 0;
  void initBlockages();
  void setTechChar(TechChar& techChar) { techChar_ = &techChar; }
  const Clock& getClock() const { return clock_; }
  Clock& getClock() { return clock_; }
  void addChild(TreeBuilder* child) { children_.emplace_back(child); }
  std::vector<TreeBuilder*> getChildren() const { return children_; }
  TreeBuilder* getParent() const { return parent_; }
  unsigned getTreeBufLevels() const { return treeBufLevels_; }
  void addFirstLevelSinkDriver(ClockInst* inst)
  {
    first_level_sink_drivers_.insert(inst);
  }
  void addSecondLevelSinkDriver(ClockInst* inst)
  {
    second_level_sink_drivers_.insert(inst);
  }
  void addTreeLevelBuffer(ClockInst* inst) { tree_level_buffers_.insert(inst); }
  bool isAnyTreeBuffer(ClockInst* inst)
  {
    return isLeafBuffer(inst) || isLevelBuffer(inst);
  }
  bool isLeafBuffer(ClockInst* inst)
  {
    return isFirstLevelSinkDriver(inst) || isSecondLevelSinkDriver(inst);
  }
  bool isFirstLevelSinkDriver(ClockInst* inst)
  {
    return first_level_sink_drivers_.find(inst)
           != first_level_sink_drivers_.end();
  }
  bool isSecondLevelSinkDriver(ClockInst* inst)
  {
    return second_level_sink_drivers_.find(inst)
           != second_level_sink_drivers_.end();
  }
  bool isLevelBuffer(ClockInst* inst)
  {
    return tree_level_buffers_.find(inst) != tree_level_buffers_.end();
  }
  void setDb(odb::dbDatabase* db) { db_ = db; }
  void setLogger(utl::Logger* logger) { logger_ = logger; }
  bool isInsideBbox(double x,
                    double y,
                    double x1,
                    double y1,
                    double x2,
                    double y2)
  {
    if ((x >= x1) && (x <= x2) && (y >= y1) && (y <= y2)) {
      return true;
    }
    return false;
  }
  bool findBlockage(const Point<double>& bufferLoc,
                    double scalingUnit,
                    double& x1,
                    double& y1,
                    double& x2,
                    double& y2);
  Point<double> legalizeOneBuffer(Point<double> bufferLoc,
                                  const std::string& bufferName);
  utl::Logger* getLogger() { return logger_; }

 protected:
  CtsOptions* options_ = nullptr;
  Clock clock_;
  TechChar* techChar_ = nullptr;
  TreeBuilder* parent_;
  std::vector<TreeBuilder*> children_;
  // Tree buffer levels. Number of buffers inserted in first leg of the HTree
  // is buffer levels (depth) of tree in all legs.
  // This becomes buffer level for whole tree thus
  unsigned treeBufLevels_ = 0;
  std::set<ClockInst*> first_level_sink_drivers_;
  std::set<ClockInst*> second_level_sink_drivers_;
  std::set<ClockInst*> tree_level_buffers_;
  utl::Logger* logger_;
  odb::dbDatabase* db_;
  std::vector<odb::dbBox*> bboxList_;
};

}  // namespace cts
