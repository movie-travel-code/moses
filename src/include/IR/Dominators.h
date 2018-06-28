//===------------------------------Dominators.h---------------------------===//
//
// This file defines the following classes:
//	1. DominatorTree: Represent dominators as an explicit tree structure.
//	2. DominatorFrontier: Calculate and hold the dominance frontier for a
//		function.
// Use the "A Simple, Fast Dominance Algorithm.Keith D.Cooper" to construct
// dominator tree.
//
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_DOMINATORS_H
#define MOSES_IR_DOMINATORS_H
#include "BasicBlock.h"
#include <map>
#include <set>

namespace compiler {
namespace IR {
class DomTreeNode;
using DomTreeNodePtr = std::shared_ptr<DomTreeNode>;

// CFGNode - Wrapper of the BasicBlock. CFGNode is the element of
// the DomTree.
class DomTreeNode {
public:
  enum class color { WHITE, GRAY, BLACK };

private:
  BBPtr TheBB;
  int PostNumber;
  int DFSInNum, DFSOutNum;
  color VisitColor;
  DomTreeNodePtr IDom;

  std::vector<DomTreeNodePtr> Children;
  // Express the predecessor when we depth-first searching of
  // the CFG.
  // e.g.
  //     B1        B2
  //      \        /
  //		 \      /
  //			B3
  // B1 and B2 both the father of B3, but we only can via one
  // node reach B3 when the depth-first searching.
  DomTreeNodePtr Father;

public:
  DomTreeNode(BBPtr BB = nullptr)
      : TheBB(BB), PostNumber(-1), DFSInNum(-1), DFSOutNum(-1), IDom(nullptr),
        Father(nullptr) {}

  BBPtr getBlock() const { return TheBB; }
  DomTreeNodePtr getIDom() const { return IDom; }

  const std::vector<DomTreeNodePtr> &getChildren() const { return Children; }

  DomTreeNodePtr addChild(DomTreeNodePtr Child) {
    Children.push_back(Child);
    return Child;
  }

  std::vector<DomTreeNodePtr> Predecessors;
  unsigned getDFSNumIn() const { return DFSInNum; }
  unsigned getDFSNumOut() const { return DFSOutNum; }
  unsigned getPostOrder() const { return PostNumber; }
  bool DominatedBy(DomTreeNodePtr other) const {
    return this->DFSInNum >= other->DFSInNum &&
           this->DFSOutNum <= other->DFSOutNum;
  }

  void setDFSInNum(int InNum) { DFSInNum = InNum; }
  void setDFSOutNum(int OutNum) { DFSOutNum = OutNum; }
  void setPostNumber(int PostOrder) { PostNumber = PostOrder; }
  void setVisitColor(color c) { VisitColor = c; }
  void setDFSFather(DomTreeNodePtr DFSFather) { Father = DFSFather; }

  color getVisitColor() const { return VisitColor; }
  size_t getNumChildren() const { return Children.size(); }
  void clearAllChildren() { Children.clear(); }

  void setIDom(DomTreeNodePtr NewIDom) { IDom = NewIDom; }
};

//===------------------------------------------------------------===//
// A dominator tree is a tree where each node's children are those
// nodes it immediately dominates.
// Because the immediate dominator is unique, it is a tree. The start
// node is the root of the tree.

// DominatorTree - This represents the forward Dominance.
class DominatorTree {
  using DomTreeNodeMapType = std::map<BBPtr, DomTreeNodePtr>;
  DomTreeNodeMapType DomTreeNodes;
  DomTreeNodePtr RootNode;

  std::vector<DomTreeNodePtr> PostOrder;
  std::vector<DomTreeNodePtr> ReversePostOrder;
  std::list<BBPtr> Vertex;

  std::map<DomTreeNodePtr, std::vector<DomTreeNodePtr>> PredecessorrsOfCFG;

  // DominanceFrontier - Represent the forward Dominance Frontier.
  std::map<DomTreeNodePtr, std::set<DomTreeNodePtr>> DominanceFrontier;
  // JoinPoints - Represent the join point(have more than two predecessors)
  // of CFG.
  std::vector<DomTreeNodePtr> JoinNodes;

private:
  void getPostOrder();
  void getReversePostOrder();

  // compute the DomTree.
  void computeDomTree(BBPtr EntryBlock);
  // ��ȡ��ǰDomNode��CFG��ǰ����Ӧ��DomTreeNode.
  std::vector<DomTreeNodePtr> getDomNodePredsFromCFG(DomTreeNodePtr Node);
  // Intersect() - This function only be using to get closest parent of A and B.
  DomTreeNodePtr Intersect(DomTreeNodePtr A, DomTreeNodePtr B);

  // Insert the frontier.
  void InsertFrontier(DomTreeNodePtr Node, DomTreeNodePtr FrontierItem);

  // ComputeDomFrontier() - Compute the forward dominance frontier.
  void ComputeDomFrontier();

public:
  // compute the DomTree of the CFG.
  void runOnCFG(std::vector<BBPtr> &BBs);
  // compute the DomTree of the Function.
  void runOnFunction(FuncPtr F);

  void ComputeDomFrontierOnCFG(std::vector<BBPtr> &BBs);
  void ComputeDomFrontierOnFunction(FuncPtr F);

  DomTreeNodePtr getDomTreeNode(BBPtr BB) const;

  // getRootNode - This returns the entry node for the CFG of the function.
  DomTreeNodePtr getRootNode() { return RootNode; }
  bool properlyDominates(DomTreeNodePtr Node) const;
  bool isReachableFromEntry(BBPtr BB) const;
  bool dominates(DomTreeNodePtr A, DomTreeNodePtr B) const;

  // printIDoms - Convert IDoms to human readable form.
  void printIDoms(std::ostream &out) const;

  // printDF - Convert Dom Frontier to human readable form.
  void printDomFrontier(std::ostream &out) const;

  void DFS(DomTreeNodePtr Node);

  // Calcuate - compute a dominator tree for the given function.
  void Calcuate();

  // dominates - Return true if A dominates B. This perform the special
  // checks necessary if A and B are in the same basic block.
  bool dominates(InstPtr A, InstPtr B) const;
};
} // namespace IR
} // namespace compiler
#endif