/*
 Copyright 2020 Tim Jammer

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "conflict_detection.h"
#include "analysis_results.h"
#include "function_coverage.h"
#include "implementation_specific.h"
#include "mpi_functions.h"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/CFG.h"

#include "debug.h"

using namespace llvm;

// do i need to export it into header?
std::vector<CallBase *> get_corresponding_wait(CallBase *call);
std::vector<CallBase *> get_scope_endings(CallBase *call);
std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>>
check_conflicts(llvm::Module &M, llvm::Function *f, bool is_send);
std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>>
check_call_for_conflict(llvm::CallBase *mpi_call, bool is_send);

bool are_calls_conflicting(llvm::CallBase *orig_call,
                           llvm::CallBase *conflict_call, bool is_send);

// gets the guarding comparision for this block if any
std::pair<Value *, bool> get_guarding_compare(llvm::CallBase *call) {
  auto *bb = call->getParent();

  if (pred_size(bb) == 1) // otherwise no single branch guard
  {
    for (BasicBlock *pred : predecessors(bb)) {
      if (auto *term = dyn_cast<BranchInst>(pred->getTerminator())) {
        if (term->isConditional()) {

          Value *guard = term->getCondition();
          bool way_to_take = false; // assume our block is the false label
          if (term->getSuccessor(0) == bb) {
            way_to_take = true; // unless proven otherwise
          }

          return std::make_pair(guard, way_to_take);
        }
      }
    }
  }

  // nothing found
  return std::make_pair(nullptr, false);
}

// if scope ending.empty: normal send without a scope
std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>>
check_call_for_conflict(CallBase *mpi_call,
                        std::vector<CallBase *> scope_endings,
                        bool is_sending) {

  std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>> conflicts;

  // tuple Instr, scope_ended,in_Ibarrier
  std::set<std::tuple<Instruction *, bool, bool>> to_check;
  std::set<BasicBlock *>
      already_checked; // be aware, that revisiting the current block might be
                       // necessary if in a loop
  std::set<CallBase *> potential_conflicts;

  Instruction *current_inst = dyn_cast<Instruction>(mpi_call);
  assert(current_inst != nullptr);
  Instruction *next_inst = current_inst->getNextNode();

  // what this function does:
  // follow all possible* code paths until
  // A a synchronization occurs
  // B a conflicting call is detected
  // C end of program (mpi finalize)
  // for A the analysis will only stop if the scope of an I... call has already
  // ended

  //* if call is within an if, we only folow the path where condition is the
  // same as in our path

  // errs()<< "Start analyzing Codepath\n";
  // mpi_call->dump();

  // if this call is within the ibarrier, We suspect its past the barrier for
  // our analysis, as it may conflict with past calls, preceding calls will
  // detect the conflict with this call nonetheless Theoretically it is allowed
  // to have multiple Ibarriers interleaved. (meaning calling Ibar, Ibar, Wait,
  // Wait) but our analysis will not cover multiple ibarriers, we only care
  // about the first encountered
  // Iallreduce has the same function a a barrier, so we do not differentiate
  // between them here
  bool in_Ibarrier = false;
  std::vector<CallBase *> i_barrier_scope_end;

  bool scope_ended = scope_endings.empty();

  auto guard_pair = get_guarding_compare(mpi_call);
  Value *guard = guard_pair.first;
  bool way_to_take = guard_pair.second;

  while (next_inst != nullptr) {
    current_inst = next_inst;
    // current_inst->dump();

    if (auto *call = dyn_cast<CallBase>(current_inst)) {
      if (is_mpi_call(call)) {
        // check if this call is conflicting or if search can be stopped as this
        // is a sync point
        // errs() << "need to check call to "
        //		<< call->getCalledFunction()->getName() << "\n";
        // ignore sync if scope has not ended yet
        if (scope_ended &&
            mpi_func->sync_functions.find(call->getCalledFunction()) !=
                mpi_func->sync_functions.end()) {

          if (call->getCalledFunction() == mpi_func->mpi_Ibarrier) {
            if (in_Ibarrier) {
              errs() << "Why do you use multiple interleaved Ibarrier's? I "
                        "don't see a usecase for it.\n";
            } else {

              assert(call->getNumArgOperands() == 2 &&
                     "MPI_Ibarrier has 2 args");
              if (get_communicator(mpi_call) == call->getArgOperand(0)) {
                if (!i_barrier_scope_end.empty()) {
                  errs() << "Warning: parsing too many Ibarriers\n"
                         << "Analysis result is still correct, but false "
                            "positives are more likely";

                } else {
                  in_Ibarrier = true;
                  i_barrier_scope_end = get_corresponding_wait(call);
                }
              }
              // else: could not prove same communicator: pretend barrier isnt
              // there for our analysis
            }
          } else if (call->getCalledFunction() == mpi_func->mpi_Iallreduce) {
            if (in_Ibarrier) {
              errs() << "Why do you use multiple interleaved Ibarrier's? I "
                        "don't see a usecase for it.\n";
            } else {

              assert(call->getNumArgOperands() == 7 &&
                     "MPI_Iallreduce has 7 args");
              if (get_communicator(mpi_call) == call->getArgOperand(5)) {
                if (!i_barrier_scope_end.empty()) {
                  errs() << "Warning: parsing too many Ibarriers\n"
                         << "Analysis result is still correct, but false "
                            "positives are more likely";

                } else {
                  in_Ibarrier = true;
                  i_barrier_scope_end = get_corresponding_wait(call);
                }
              }
              // else: could not prove same communicator: pretend barrier isnt
              // there for our analysis
            }
          } else if (call->getCalledFunction() == mpi_func->mpi_barrier) {
            assert(call->getNumArgOperands() == 1 && "MPI_Barrier has 1 args");
            if (get_communicator(mpi_call) == call->getArgOperand(0)) {

              current_inst = nullptr;
              Debug(errs() << "call to " << call->getCalledFunction()->getName()
                           << " is a sync point, no overtaking possible beyond "
                              "it\n";);
            }
            // else: could not prove that barrier is in the same communicator:
            // continue analysis
          } else if (call->getCalledFunction() == mpi_func->mpi_allreduce) {
            assert(call->getNumArgOperands() == 6 &&
                   "MPI_Allreduce has 6 args");
            if (get_communicator(mpi_call) == call->getArgOperand(5)) {

              current_inst = nullptr;
              Debug(errs() << "call to " << call->getCalledFunction()->getName()
                           << " is a sync point, no overtaking possible beyond "
                              "it\n";);
            }
            // else: could not prove that barrier is in the same communicator:
            // continue analysis

          } else if (call->getCalledFunction() == mpi_func->mpi_finalize) {
            // no mpi beyond this
            current_inst = nullptr;
            Debug(
                errs()
                    << "call to " << call->getCalledFunction()->getName()
                    << " is a sync point, no overtaking possible beyond it\n";);
          }

          // no need to analyze this path further, a sync point will stop msg
          // overtaking anyway

        } else if (mpi_func->conflicting_functions.find(
                       call->getCalledFunction()) !=
                   mpi_func->conflicting_functions.end()) {
          potential_conflicts.insert(call);
        } else if (mpi_func->unimportant_functions.find(
                       call->getCalledFunction()) !=
                   mpi_func->unimportant_functions.end()) {

          if (in_Ibarrier &&
              std::find(i_barrier_scope_end.begin(), i_barrier_scope_end.end(),
                        call) != i_barrier_scope_end.end()) {

            assert(scope_ended);
            // no mpi beyond this
            current_inst = nullptr;
            Debug(errs() << "Completed Ibarrier, no overtaking possible beyond "
                            "it\n";);
          }

          if (!scope_ended &&
              std::find(scope_endings.begin(), scope_endings.end(), call) !=
                  scope_endings.end()) {
            // found end of scope
            scope_ended = true;
          }

          // errs() << "call to " << call->getCalledFunction()->getName()
          //       << " is currently not supported in this analysis\n";
        }

      } else { // no mpi function

        if (function_metadata->may_conflict(call->getCalledFunction())) {
          Debug(errs() << "Call To " << call->getCalledFunction()->getName()
                       << "May conflict\n";);
          conflicts.push_back(std::make_pair(mpi_call, call));
        } else if (function_metadata->will_sync(call->getCalledFunction())) {
          // sync point detected
          current_inst = nullptr;
          Debug(errs() << "call to " << call->getCalledFunction()->getName()
                       << " will sync, no overtaking possible beyond it\n";);
        } else if (function_metadata->is_unknown(call->getCalledFunction())) {
          Debug(
              errs()
                  << "Could not determine if call to "
                  << call->getCalledFunction()->getName()
                  << "will result in a conflict, for safety we will assume it "
                     "does \n";);
          // assume conflict
          conflicts.push_back(std::make_pair(mpi_call, call));
        }
      }
    } // end if CallBase

    // now fetch the next inst
    next_inst = nullptr;

    if (current_inst !=
        nullptr) // if not discovered to stop analyzing this code path
    {
      if (current_inst->isTerminator()) {
        if (auto *br = dyn_cast<BranchInst>(current_inst)) {
          if (br->isConditional() && br->getCondition() == guard) {
            // only add the corresponding successor

            auto *next_block = current_inst->getSuccessor(1);
            if (way_to_take) {
              next_block = current_inst->getSuccessor(0);
            }

            if (already_checked.find(next_block) == already_checked.end()) {
              to_check.insert(std::make_tuple(next_block->getFirstNonPHI(),
                                              scope_ended, in_Ibarrier));
            }

          } else {
            // add all sucessors
            for (unsigned int i = 0; i < current_inst->getNumSuccessors();
                 ++i) {
              auto *next_block = current_inst->getSuccessor(i);

              if (already_checked.find(next_block) == already_checked.end()) {
                to_check.insert(std::make_tuple(next_block->getFirstNonPHI(),
                                                scope_ended, in_Ibarrier));
              }
            }
          }
        } else { // e.g. some switch
                 // add all sucessors
          for (unsigned int i = 0; i < current_inst->getNumSuccessors(); ++i) {
            auto *next_block = current_inst->getSuccessor(i);

            if (already_checked.find(next_block) == already_checked.end()) {
              to_check.insert(std::make_tuple(next_block->getFirstNonPHI(),
                                              scope_ended, in_Ibarrier));
            }
          }
        }
        if (isa<ReturnInst>(current_inst)) {
          // we have to check all exit points of this function for conflicts as
          // well...
          Function *f = current_inst->getFunction();
          for (auto *user : f->users()) {
            if (auto *where_returns = dyn_cast<CallBase>(user)) {
              if (where_returns->getCalledFunction() == f) {
                assert(where_returns->getNextNode() != nullptr);
                to_check.insert(std::make_tuple(where_returns->getNextNode(),
                                                scope_ended, in_Ibarrier));
              }
            }
          }
        }
      }

      next_inst = current_inst->getNextNode();
    }

    if (dyn_cast_or_null<UnreachableInst>(next_inst)) { // stop at unreachable
      next_inst = nullptr;
    }

    if (next_inst == nullptr) {
      // errs() << to_check.size();
      if (!to_check.empty()) {
        auto it_pos = to_check.begin();

        std::tuple<Instruction *, bool, bool> tup = *it_pos;
        next_inst = std::get<0>(tup);
        scope_ended = std::get<1>(tup);
        in_Ibarrier = std::get<2>(tup);
        to_check.erase(it_pos);
        already_checked.insert(
            next_inst->getParent()); // will be checked now, so no revisiting
                                     // necessary
      }
    }
  } // end while

  // TODO: std::filter
  // check for conflicts:
  for (auto *call : potential_conflicts) {
    bool conflict = are_calls_conflicting(mpi_call, call, is_sending);
    if (conflict) {
      // found at least one conflict, currently we can stop then
      conflicts.push_back(std::make_pair(mpi_call, call));
    }
  }

  // no conflict found
  return conflicts;
}

std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>>
check_conflicts(llvm::Module &M, llvm::Function *f, bool is_sending) {
  if (f == nullptr) {
    // no messages: no conflict: return empty vector
    return {};
  }
  std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>> result;

  for (auto user : f->users()) {
    if (CallBase *call = dyn_cast<CallBase>(user)) {
      if (call->getCalledFunction() == f) {
        auto scope_endings = get_scope_endings(call);
        auto temp = check_call_for_conflict(call, scope_endings, is_sending);
        result.insert(result.end(), temp.begin(), temp.end());

      } else {
        call->dump();
        errs() << "\nWhy do you do that?\n";
      }
    }
  }

  return result;
}

std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>>
check_mpi_send_conflicts(Module &M) {
  std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>> result;
  auto temp = check_conflicts(M, mpi_func->mpi_send, true);
  result.insert(result.end(), temp.begin(), temp.end());

  //  check_conflicts(M,mpi_func->mpi_Ssend);
  // Ssend may not yield to conflicts regarding overtaking messages:
  // when Ssend returns: the receiver have begun execution of the matching recv
  // therefore further send operation may not overtake this one

  // this ssend can overtake another send ofc but this conflict will be handled
  // when the overtakend send is analyzed

  //  check_conflicts(M, mpi_func->mpi_Rsend);

  // standard:
  // A send operation that uses the ready mode has the same semantics as a
  // standard send operation, or a synchronous send operation; it is merely that
  // the sender provides additional information to the system (namely that a
  // matching receive is already posted), that can save some overhead. In a
  // correct program, therefore, a ready send couldbe replaced by a standard
  // send with no effect on the behavior of the program other than performance.

  // This means, the sender have started to execute the matching send, therefore
  // it has the same as Ssend.

  temp = check_conflicts(M, mpi_func->mpi_Bsend, true);
  result.insert(result.end(), temp.begin(), temp.end());

  temp = check_conflicts(M, mpi_func->mpi_Isend, true);
  result.insert(result.end(), temp.begin(), temp.end());

  // sending part of sendrecv
  temp = check_conflicts(M, mpi_func->mpi_Sendrecv, true);
  result.insert(result.end(), temp.begin(), temp.end());

  return result;
}

std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>>
check_mpi_recv_conflicts(Module &M) {
  std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>> result;

  auto temp = check_conflicts(M, mpi_func->mpi_recv, false);
  result.insert(result.end(), temp.begin(), temp.end());

  // recv part of sendrecv
  temp = check_conflicts(M, mpi_func->mpi_Sendrecv, false);
  result.insert(result.end(), temp.begin(), temp.end());

  temp = check_conflicts(M, mpi_func->mpi_Irecv, false);
  result.insert(result.end(), temp.begin(), temp.end());

  return result;
}

bool can_prove_val_different(Value *val_a, Value *val_b,
                             bool check_for_loop_iter_difference);

// if at least one value is inside a loop, this function ties to prove that the
// two values differ for every iteration of the loop
// e.g. take in account the loop boundaries
bool can_prove_val_different_respecting_loops(Value *val_a, Value *val_b) {

  auto *inst_a = dyn_cast<Instruction>(val_a);
  auto *inst_b = dyn_cast<Instruction>(val_b);
  if (!inst_a && inst_b) {
    return can_prove_val_different_respecting_loops(val_b, val_a);
  } else if (!inst_a && !inst_b) {
    return false;
  }

  assert(inst_a);

  LoopInfo *linfo = analysis_results->getLoopInfo(inst_a->getFunction());
  ScalarEvolution *se = analysis_results->getSE(inst_a->getFunction());
  assert(linfo != nullptr && se != nullptr);

  // Debug(errs() << "try to prove difference within loop\n";)

  auto *sc_a = se->getSCEV(val_a);
  auto *sc_b = se->getSCEV(val_b);

  // Debug(sc_a->print(errs()); errs() << "\n"; sc_b->print(errs());errs() <<
  // "\n";)

  bool result = se->isKnownPredicate(CmpInst::Predicate::ICMP_NE, sc_a, sc_b);
  /*Debug(if (result) {errs() << "Known different\n";} else {
   errs() << "could not prove difference\n";
   })*/

  return result;
}

// this function tries to prove if the given values differ for different loop
// iterations
bool can_prove_val_different_for_different_loop_iters(Value *val_a,
                                                      Value *val_b) {

  if (val_a != val_b) {
    return false;
  }

  assert((!isa<Constant>(val_a) || !isa<Constant>(val_b)) &&
         "This function should not be used with two constants");
  auto *inst_a = dyn_cast<Instruction>(val_a);
  auto *inst_b = dyn_cast<Instruction>(val_b);

  if (inst_b && !inst_a) {
    // we assume first param is an insruction (second may be constant)
    return can_prove_val_different_for_different_loop_iters(val_b, val_a);
  }

  assert(inst_a && "This should be an Instruction");
  assert(inst_a->getType()->isIntegerTy());

  LoopInfo *linfo = analysis_results->getLoopInfo(inst_a->getFunction());
  ScalarEvolution *se = analysis_results->getSE(inst_a->getFunction());
  assert(linfo != nullptr && se != nullptr);
  Loop *loop = linfo->getLoopFor(inst_a->getParent());

  if (loop) {
    auto *sc = se->getSCEV(inst_a);

    // if we can prove that the variable varies predictably with the loop, the
    // variable will be different for any two loop iterations otherwise the
    // variable is only LoopVariant but not predictable
    if (se->getLoopDisposition(sc, loop) ==
        ScalarEvolution::LoopDisposition::LoopComputable) {
      auto *sc_2 = se->getSCEV(val_b);
      // if vals are the same and predicable throuout the loop they differ each
      // iteration
      if (se->isKnownPredicate(CmpInst::Predicate::ICMP_EQ, sc, sc_2)) {
        assert(se->getLoopDisposition(sc, loop) !=
               ScalarEvolution::LoopDisposition::LoopInvariant);
        return true;

      } else {
        sc->print(errs());
        sc_2->print(errs());
      }
    }
  }

  return false;
}

// TODO this analysis may not work if a thread gets a pointer to another
// thread's stack, but whoever does that is dumb anyway...
bool can_prove_val_different(Value *val_a, Value *val_b,
                             bool check_for_loop_iter_difference) {

  // errs() << "Comparing: \n";
  // val_a->dump();
  // val_b->dump();

  if (val_a->getType() != val_b->getType()) {
    // this should not happen anyway
    assert(false && "Trying comparing values of different types.");
    return true;
  }

  if (auto *c1 = dyn_cast<Constant>(val_a)) {
    if (auto *c2 = dyn_cast<Constant>(val_b)) {
      if (c1 != c2) {
        // different constants
        // errs() << "Different\n";
        return true;
      } else {
        // proven same
        return false;
      }
    }
  }

  if (can_prove_val_different_respecting_loops(val_a, val_b)) {
    return true;
  }

  if (check_for_loop_iter_difference) {
    if (can_prove_val_different_for_different_loop_iters(val_a, val_b)) {
      return true;
    }
  }

  // could not prove difference
  return false;
}

// true if there is a path from current pos containing block1 and then block2
// (and all blocks are in the loop) depth first search through the loop
bool is_path_in_loop_iter(BasicBlock *current_pos, bool encountered1,
                          Loop *loop, BasicBlock *block1, BasicBlock *block2,
                          std::set<BasicBlock *> &visited) {

  // end
  if (current_pos == block2) {
    return encountered1;
    // if we first discover block2 and then block 1 this does not count
  }

  bool has_encountered_1 = encountered1;
  if (current_pos == block1) {
    has_encountered_1 = true;
  }

  visited.insert(current_pos);

  auto *term = current_pos->getTerminator();

  for (unsigned int i = 0; i < term->getNumSuccessors(); ++i) {
    auto *next = term->getSuccessor(i);
    if (loop->contains(next) && visited.find(next) == visited.end()) {
      // do not leave one loop iter
      if (is_path_in_loop_iter(next, has_encountered_1, loop, block1, block2,
                               visited)) {
        // found path
        return true;
      }
    }
    // else search for other paths
  }

  // no path found
  return false;
}

// TODO does not work for all cases

// true if both calls are in the same loop and there is no loop iterations where
// both calls are called
bool are_calls_in_different_loop_iters(CallBase *orig_call,
                                       CallBase *conflict_call) {

  if (orig_call == conflict_call) {
    // call conflicting with itself may only be bart of one loop
    return true;
  }

  if (orig_call->getFunction() != conflict_call->getFunction()) {

    return false;
  }

  LoopInfo *linfo = analysis_results->getLoopInfo(orig_call->getFunction());
  assert(linfo != nullptr);

  Loop *loop = linfo->getLoopFor(orig_call->getParent());

  if (!loop) { // not in loop
    return false;
  }

  if (loop != linfo->getLoopFor(conflict_call->getParent())) {
    // if in different loops or one is not in a loop
    return false;
  }

  assert(loop != nullptr &&
         loop == linfo->getLoopFor(conflict_call->getParent()));

  BasicBlock *orig_block = orig_call->getParent();
  BasicBlock *confilct_block = conflict_call->getParent();

  if (orig_block == confilct_block) {
    // obvious: true on every loop iteration
    return true;
  }

  // depth first search from the loop header to find a path through the
  // iteration containing both calls

  std::set<BasicBlock *> visited = {}; // start with empty set
  return !is_path_in_loop_iter(loop->getHeader(), false, loop, orig_block,
                               confilct_block, visited);
}

bool are_calls_conflicting(CallBase *orig_call, CallBase *conflict_call,
                           bool is_send) {

  // if one is send and the other a recv: fond a match which means no conflict
  if ((is_send && is_recv_function(conflict_call->getCalledFunction())) ||
      (!is_send && is_send_function(conflict_call->getCalledFunction()))) {
    return false;
  }

  // if true: proven to be different if the loop induction variable(s) are
  // different means a proven difference if false we do not consider a
  // difference between different loop iterations
  bool check_for_loop_iter_difference = false;

  if (are_calls_in_different_loop_iters(orig_call, conflict_call)) {
    check_for_loop_iter_difference = true;
  }

  // check communicator
  auto *comm1 = get_communicator(orig_call);
  auto *comm2 = get_communicator(conflict_call);
  if (can_prove_val_different(comm1, comm2, check_for_loop_iter_difference)) {
    return false;
  }
  // otherwise, we have not proven that the communicator is be different
  // TODO: (very advanced) if e.g. mpi comm split is used, we might be able to
  // statically prove different communicators

  // check src
  auto *src1 = get_src(orig_call, is_send);
  auto *src2 = get_src(conflict_call, is_send);
  if (can_prove_val_different(src1, src2, check_for_loop_iter_difference)) {
    return false;
  }
  if (src1 == mpi_implementation_specifics->ANY_SOURCE &&
      src2 == mpi_implementation_specifics->ANY_SOURCE) {
    // if any src is used, the order is not defined in the first place (as the
    // matching is nondeterministically)
    return false;
  }

  // check tag
  auto *tag1 = get_tag(orig_call, is_send);
  auto *tag2 = get_tag(conflict_call, is_send);
  if (can_prove_val_different(tag1, tag2, check_for_loop_iter_difference)) {
    return false;
  } // otherwise, we have not proven that the tag is be different
  if (tag1 == mpi_implementation_specifics->ANY_TAG &&
      tag2 == mpi_implementation_specifics->ANY_TAG) {
    // if any tag is used, the order is not defined in the first place (as the
    // matching is nondeterministically)
    return false;
  }

  // cannot disprove conflict, have to assume it indeed relays on msg ordering
  return true;
}

std::vector<CallBase *> get_scope_endings(CallBase *call) {

  auto *F = call->getCalledFunction();
  if (F == mpi_func->mpi_Irecv || F == mpi_func->mpi_Isend ||
      F == mpi_func->mpi_Iallreduce || F == mpi_func->mpi_Ibarrier ||
      F == mpi_func->mpi_Issend) {
    return get_corresponding_wait(call);
  } else if (F == mpi_func->mpi_Bsend ||
             F == mpi_func->mpi_Ibsend) { // search for all mpi buffer detach

    std::vector<CallBase *> scope_endings;
    for (auto *user : mpi_func->mpi_buffer_detach->users()) {
      if (auto *buffer_detach_call = dyn_cast<CallBase>(user)) {
        assert(buffer_detach_call->getCalledFunction() ==
               mpi_func->mpi_buffer_detach);
        scope_endings.push_back(buffer_detach_call);
      }
    }
    return scope_endings;
  } else {
    // n I.. call: no scope ending
    return {};
  }
}

bool is_waitall_matching(ConstantInt *begin_index, ConstantInt *match_index,
                         CallBase *call) {
  assert(begin_index->getType() == match_index->getType());
  assert(call->getCalledFunction() == mpi_func->mpi_waitall);
  assert(call->getNumArgOperands() == 3);

  Debug(call->dump(); errs() << "Is this waitall matching?";);

  if (auto *count = dyn_cast<ConstantInt>(call->getArgOperand(0))) {

    auto begin = begin_index->getSExtValue();
    auto match = match_index->getSExtValue();
    auto num_req = count->getSExtValue();

    if (begin + num_req > match && match >= begin) {
      // proven, that this request is part of the requests waited for by the
      // call
      Debug(errs() << "  TRUE\n";);
      return true;
    }
  }

  // could not prove true
  Debug(errs() << "  FALSE\n";);
  return false;
}

std::vector<CallBase *> get_matching_waitall(AllocaInst *request_array,
                                             ConstantInt *index) {
  std::vector<CallBase *> result;

  for (auto u : request_array->users()) {
    if (auto *call = dyn_cast<CallBase>(u)) {
      if (call->getCalledFunction() == mpi_func->mpi_wait && index->isZero()) {
        // may use wait like this
        result.push_back(call);
      }
      if (call->getCalledFunction() == mpi_func->mpi_waitall) {
        if (is_waitall_matching(ConstantInt::get(index->getType(), 0), index,
                                call)) {
          result.push_back(call);
        }
      }
    } else if (auto *gep = dyn_cast<GetElementPtrInst>(u)) {
      if (gep->getNumIndices() == 2 && gep->hasAllConstantIndices()) {
        auto *index_it = gep->idx_begin();
        ConstantInt *i0 = dyn_cast<ConstantInt>(&*index_it);
        index_it++;
        ConstantInt *index_in_array = dyn_cast<ConstantInt>(&*index_it);
        if (i0->isZero()) {

          for (auto u2 : gep->users()) {
            if (auto *call = dyn_cast<CallBase>(u2)) {
              if (call->getCalledFunction() == mpi_func->mpi_wait &&
                  index == index_in_array) {
                // may use wait like this
                result.push_back(call);
              }
              if (call->getCalledFunction() == mpi_func->mpi_waitall) {
                if (is_waitall_matching(index_in_array, index, call)) {
                  result.push_back(call);
                }
              }
            }
          }

        } // end if index0 == 0
      }   // end if gep has simple structure
    }
  }

  return result;
}

std::vector<CallBase *> get_corresponding_wait(CallBase *call) {

  // errs() << "Analyzing scope of \n";
  // call->dump();

  std::vector<CallBase *> result;
  unsigned int req_arg_pos = 6;
  if (call->getCalledFunction() == mpi_func->mpi_Ibarrier) {
    assert(call->getNumArgOperands() == 2);
    req_arg_pos = 1;
  } else {
    assert(call->getCalledFunction() == mpi_func->mpi_Isend ||
           call->getCalledFunction() == mpi_func->mpi_Ibsend ||
           call->getCalledFunction() == mpi_func->mpi_Issend ||
           call->getCalledFunction() == mpi_func->mpi_Irsend ||
           call->getCalledFunction() == mpi_func->mpi_Irecv ||
           call->getCalledFunction() == mpi_func->mpi_Iallreduce);
    assert(call->getNumArgOperands() == 7);
  }

  Value *req = call->getArgOperand(req_arg_pos);

  // req->dump();
  if (auto *alloc = dyn_cast<AllocaInst>(req)) {
    for (auto *user : alloc->users()) {
      if (auto *other_call = dyn_cast<CallBase>(user)) {
        if (other_call->getCalledFunction() == mpi_func->mpi_wait) {
          assert(other_call->getNumArgOperands() == 2);
          assert(other_call->getArgOperand(0) == req &&
                 "First arg of MPi wait is MPI_Request");
          // found end of scope
          // errs() << "possible ending of scope here \n";
          // other_call->dump();
          result.push_back(other_call);
        }
      }
    }
  }
  // scope detection in basic waitall
  // ofc at some point of pointer arithmetic, we cannot follow it
  else if (auto *gep = dyn_cast<GetElementPtrInst>(req)) {
    if (gep->isInBounds()) {
      if (auto *req_array = dyn_cast<AllocaInst>(gep->getPointerOperand())) {
        if (gep->getNumIndices() == 2 && gep->hasAllConstantIndices()) {

          auto *index_it = gep->idx_begin();
          ConstantInt *i0 = dyn_cast<ConstantInt>(&*index_it);
          index_it++;
          ConstantInt *index_in_array = dyn_cast<ConstantInt>(&*index_it);
          if (i0->isZero()) {

            auto temp = get_matching_waitall(req_array, index_in_array);
            result.insert(result.end(), temp.begin(), temp.end());

          } // end it index0 == 0
        }   // end if gep has a simple structure
        else {
          // debug
          Debug(gep->dump();
                errs()
                << "This structure is currently too complicated to analyze";);
        }
      }

    } else { // end if inbounds
      gep->dump();
      errs() << "Strange, out of bounds getelemptr instruction should not "
                "happen in this case\n";
    }
  }

  if (result.empty()) {
    errs() << "could not determine scope of \n";
    call->dump();
    errs() << "Assuming it will finish at mpi_finalize.\n"
           << "The Analysis result is still valid, although the chance of "
              "false positives is higher\n";
  }

  // mpi finalize will end all communication nontheles
  for (auto *user : mpi_func->mpi_finalize->users()) {
    if (auto *finalize_call = dyn_cast<CallBase>(user)) {
      assert(finalize_call->getCalledFunction() == mpi_func->mpi_finalize);
      result.push_back(finalize_call);
    }
  }

  return result;
}

Value *get_communicator(CallBase *mpi_call) {

  unsigned int total_num_args = 0;
  unsigned int communicator_arg_pos = 0;

  if (mpi_call->getCalledFunction() == mpi_func->mpi_send ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Bsend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Ssend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Rsend) {
    total_num_args = 6;
    communicator_arg_pos = 5;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Isend) {
    total_num_args = 7;
    communicator_arg_pos = 5;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_recv ||
             mpi_call->getCalledFunction() == mpi_func->mpi_Irecv) {
    total_num_args = 7;
    communicator_arg_pos = 5;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Sendrecv) {
    total_num_args = 12;
    communicator_arg_pos = 10;
  } else {
    errs() << mpi_call->getCalledFunction()->getName()
           << ": This MPI function is currently not supported\n";
    assert(false);
  }

  assert(mpi_call->getNumArgOperands() == total_num_args);

  return mpi_call->getArgOperand(communicator_arg_pos);
}

Value *get_src(CallBase *mpi_call, bool is_send) {

  unsigned int total_num_args = 0;
  unsigned int src_arg_pos = 0;

  if (mpi_call->getCalledFunction() == mpi_func->mpi_send ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Bsend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Ssend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Rsend) {
    assert(is_send);
    total_num_args = 6;
    src_arg_pos = 3;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Isend) {
    assert(is_send);
    total_num_args = 7;
    src_arg_pos = 3;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_recv ||
             mpi_call->getCalledFunction() == mpi_func->mpi_Irecv) {
    assert(!is_send);
    total_num_args = 7;
    src_arg_pos = 3;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Sendrecv) {
    total_num_args = 12;
    if (is_send)
      src_arg_pos = 3;
    else
      src_arg_pos = 8;
  } else {
    errs() << mpi_call->getCalledFunction()->getName()
           << ": This MPI function is currently not supported\n";
    assert(false);
  }

  assert(mpi_call->getNumArgOperands() == total_num_args);

  return mpi_call->getArgOperand(src_arg_pos);
}

Value *get_tag(CallBase *mpi_call, bool is_send) {

  unsigned int total_num_args = 0;
  unsigned int tag_arg_pos = 0;

  if (mpi_call->getCalledFunction() == mpi_func->mpi_send ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Bsend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Ssend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Rsend) {
    assert(is_send);
    total_num_args = 6;
    tag_arg_pos = 4;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Isend) {
    assert(is_send);
    total_num_args = 7;
    tag_arg_pos = 4;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_recv ||
             mpi_call->getCalledFunction() == mpi_func->mpi_Irecv) {
    assert(!is_send);
    total_num_args = 7;
    tag_arg_pos = 4;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Sendrecv) {
    total_num_args = 12;
    if (is_send)
      tag_arg_pos = 4;
    else
      tag_arg_pos = 9;
  } else {
    errs() << mpi_call->getCalledFunction()->getName()
           << ": This MPI function is currently not supported\n";
    assert(false);
  }

  assert(mpi_call->getNumArgOperands() == total_num_args);

  return mpi_call->getArgOperand(tag_arg_pos);
}
