//===-----------------------------Dominators.cpp--------------------------===//
//
// This file implements the following classes:
//	1. DominatorTree: Represent dominators as an explicit tree structure.
//	2. DominatorFrontier: Calculate and hold the dominance frontier for a
//		function.
// Use the "A Simple, Fast Dominance Algorithm.Keith D.Cooper" to construct
// dominator tree.
//
//===---------------------------------------------------------------------===//
#include "IR/Dominators.h"
using namespace IR;
using color = DomTreeNode::color;

//===---------------------------------------------------------------------===//
// Implements the class DominatorTree
DomTreeNodePtr DominatorTree::getDomTreeNode(std::shared_ptr<BasicBlock> BB) const {
  auto Result = DomTreeNodes.find(BB);
  if (Result != DomTreeNodes.end())
    return Result->second;
  assert(0 && "Unreachable code.");
  return nullptr;
}

void DominatorTree::printIDoms(std::ostream &out) const {
  for (auto Node : DomTreeNodes) {
    out << "IDom(" << Node.second->getBlock()->getName()
        << "): " << Node.second->getIDom()->getBlock()->getName() << std::endl;
  }
}

void DominatorTree::printDomFrontier(std::ostream &out) const {
  for (const auto &item : DominanceFrontier) {
    out << "DF(" << item.first->getBlock()->getName() << "): ";
    for (auto frontier : item.second) {
      out << frontier->getBlock()->getName() << " ";
    }
    out << std::endl;
  }

  std::cout << "---------------------------------------------------------------"
               "-----------------\n";
}

void DominatorTree::runOnCFG(std::vector<std::shared_ptr<BasicBlock>> &BBs) {
  for (const auto &item : BBs)
    Vertex.push_back(item);
  computeDomTree(BBs[0]);
}

void DominatorTree::runOnFunction(std::shared_ptr<Function> F) {
  Vertex = F->getBasicBlockList();
  computeDomTree(F->getEntryBlock());
}

// compute the DomTree.
void DominatorTree::computeDomTree(std::shared_ptr<BasicBlock> EntryBlock) {
  for (const auto &item : Vertex) {
    DomTreeNodes.insert({item, std::make_shared<DomTreeNode>(item)});
  }

  auto FindEntry = getDomTreeNode(EntryBlock);
  assert(FindEntry && "must have entry node");
  RootNode = FindEntry;

  // (2) DFS() �õ� PostOrder �Լ� ReversePostOrder
  for (auto item : DomTreeNodes)
    item.second->setVisitColor(color::WHITE);
  DFS(RootNode);

  // (3) Dominance Info.
  Calcuate();

  // (4) print
  printIDoms(std::cout);
}

// DFS() - We will get post-order and reverse post-order of CFG through
// Depth-first searching.
void DominatorTree::DFS(DomTreeNodePtr Node) {
  std::string name = Node->getBlock()->getName();
  static int DFSCounter = 0;
  static int PostOrder = 0;
  Node->setDFSInNum(DFSCounter++);
  Node->setVisitColor(color::GRAY);

  // Get the successors of the Node.
  auto TermiInst = Node->getBlock()->getTerminator();
  if (TermiInst == nullptr) {
    Node->setDFSOutNum(DFSCounter++);
    Node->setPostNumber(PostOrder++);
    Node->setVisitColor(color::BLACK);
    return;
  }

  std::vector<DomTreeNodePtr> SuccNodes;
  for (unsigned i = 0, size = TermiInst->getNumSuccessors(); i < size; i++) {
    SuccNodes.push_back(getDomTreeNode(TermiInst->getSuccessor(i)));
  }

  for (const auto &item : SuccNodes) {
    if (item->getVisitColor() == color::WHITE) {
      item->setDFSFather(Node);
      DFS(item);
    }
  }
  Node->setDFSOutNum(DFSCounter++);
  Node->setPostNumber(PostOrder++);
  Node->setVisitColor(color::BLACK);
}

// To Do: optimization
void DominatorTree::getPostOrder() {
  for (const auto &item : DomTreeNodes)
    item.second->setVisitColor(color::WHITE);

  for (std::size_t i = 0, size = DomTreeNodes.size(); i < size; ++i) {
    size_t flag = DomTreeNodes.size();
    DomTreeNodePtr tmp = nullptr;

    for (const auto &item : DomTreeNodes) {
      if (item.second->getPostOrder() < flag &&
          item.second->getVisitColor() != color::BLACK) {
        flag = item.second->getPostOrder();
        tmp = item.second;
      }
    }

    tmp->setVisitColor(color::BLACK);
    PostOrder.push_back(tmp);
  }
}

void DominatorTree::getReversePostOrder() {
  if (PostOrder.size() == 0)
    getPostOrder();
  for (std::size_t i = PostOrder.size(); i >= 1; --i)
    ReversePostOrder.push_back(PostOrder[i - 1]);
}

// Get the predecessors of the \parm Node.
std::vector<DomTreeNodePtr>
DominatorTree::getDomNodePredsFromCFG(DomTreeNodePtr Node) {
  auto Result = PredecessorrsOfCFG.find(Node);
  if (Result != PredecessorrsOfCFG.end())
    return Result->second;

  std::vector<DomTreeNodePtr> PredDomTreeNode;
  auto BB = Node->getBlock();
  auto Preds = BB->getPredecessors();

  for (const auto &pred : Preds)
    PredDomTreeNode.push_back(getDomTreeNode(pred));
  if (Preds.size() > 1)
    JoinNodes.push_back(Node);
  PredecessorrsOfCFG.insert({Node, PredDomTreeNode});
  return PredDomTreeNode;
}

DomTreeNodePtr DominatorTree::Intersect(DomTreeNodePtr A, DomTreeNodePtr B) {
  DomTreeNodePtr finger1 = A;
  DomTreeNodePtr finger2 = B;

  while (finger1 != finger2) {
    while (finger1->getPostOrder() < finger2->getPostOrder())
      finger1 = finger1->getIDom();
    while (finger2->getPostOrder() < finger1->getPostOrder())
      finger2 = finger2->getIDom();
  }
  return finger1;
}

void DominatorTree::Calcuate() {
  if (ReversePostOrder.size() == 0)
    getReversePostOrder();

  // iterate
  bool changed = true;
  RootNode->setIDom(RootNode);

  while (changed) {
    changed = false;
    for (const auto &CurNode : ReversePostOrder) {
      if (CurNode == RootNode)
        continue;

      // Get the predecessors of current node.
      auto PredDomNodeFromCFG = getDomNodePredsFromCFG(CurNode);

      // (1) Find the first non-nullptr predecessor.
      auto getAvailiablePred = [&PredDomNodeFromCFG]() -> DomTreeNodePtr {
        for (auto pred : PredDomNodeFromCFG) {
          if (pred->getIDom() != nullptr)
            return pred;
        }
        assert(0 && "Unreachable code.");
        return nullptr;
      };

      auto AvailiablePred = getAvailiablePred();
      DomTreeNodePtr NewIDom = AvailiablePred;

      // (2) Traverse other predecessors.
      for (const auto &pred : PredDomNodeFromCFG) {
        if (pred == NewIDom)
          continue;
        if (pred->getIDom() != nullptr)
          NewIDom = Intersect(NewIDom, pred);
      }

      // (3) Judge the IDom is changed.
      if (CurNode->getIDom() != NewIDom) {
        CurNode->setIDom(NewIDom);
        changed = false;
      }
    }
  }
}

void DominatorTree::InsertFrontier(DomTreeNodePtr Node,
                                   DomTreeNodePtr FrontierItem) {
  auto NodeEntry = DominanceFrontier.find(Node);
  if (NodeEntry != DominanceFrontier.end()) {
    NodeEntry->second.insert(FrontierItem);
  } else {
    std::set<DomTreeNodePtr> Frontier;
    Frontier.insert(FrontierItem);
    DominanceFrontier.insert({Node, Frontier});
  }
}

// Compute the forward dominance frontier(Use Cooper's algorithm).
// The algorithm to compute the dominance frontier.
void DominatorTree::ComputeDomFrontier() {
  DomTreeNodePtr runner = nullptr;
  // Just compute the join points.
  for (const auto &Node : JoinNodes) {
    auto preds = getDomNodePredsFromCFG(Node);
    for (auto pred : preds) {
      runner = pred;
      while (runner != Node->getIDom()) {
        InsertFrontier(runner, Node);
        runner = runner->getIDom();
      }
    }
  }
}

void DominatorTree::ComputeDomFrontierOnCFG(std::vector<std::shared_ptr<BasicBlock>> &BBs) {
  if (RootNode->getIDom() == nullptr)
    runOnCFG(BBs);
  ComputeDomFrontier();

  printDomFrontier(std::cout);
}
void DominatorTree::ComputeDomFrontierOnFunction(std::shared_ptr<Function> F) {
  if (RootNode->getIDom() == nullptr)
    runOnFunction(F);
  ComputeDomFrontier();

  printDomFrontier(std::cout);
}
