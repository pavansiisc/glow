// Copyright 2017 Facebook Inc.  All Rights Reserved.

#include "glow/Graph/Nodes.h"
#include "glow/Base/Type.h"
#include "glow/IR/Instrs.h"
#include "glow/Support/Support.h"

using namespace glow;

void NodeUse::setOperand(Node *other) {
  if (other && site_->get()) {
    assert(site_->get()->getType() == other->getType() &&
           "Setting operand to a node with a different type");
  }
  site_->setOperand(other);
}

void NodeOperand::setOperand(Node *v) {
  if (node_ == v) {
    return;
  }

  if (node_) {
    node_->removeUse(NodeUse(this));
    node_ = nullptr;
  }

  if (v) {
    node_ = v;
    v->addUse(NodeUse(this));
  }
}

//===----------------------------------------------------------------------===//
//                        Visitor methods
//===----------------------------------------------------------------------===//

void Variable::visit(Node *parent, NodeVisitor *visitor) {
  if (!visitor->shouldVisit(parent, this)) {
    return;
  }
  visitor->pre(parent, this);
  visitor->post(parent, this);
}

#define DEFINE_CLASS_VISITOR(CLASS_NAME)                                       \
  void CLASS_NAME::visit(Node *parent, NodeVisitor *visitor) {                 \
    if (!visitor->shouldVisit(parent, this))                                   \
      return;                                                                  \
    visitor->pre(parent, this);                                                \
    in_->visit(this, visitor);                                                 \
    visitor->post(parent, this);                                               \
  }

DEFINE_CLASS_VISITOR(PoolNode)
DEFINE_CLASS_VISITOR(LocalResponseNormalizationNode)
DEFINE_CLASS_VISITOR(ReluNode)
DEFINE_CLASS_VISITOR(ReshapeNode)
DEFINE_CLASS_VISITOR(TransposeNode)
DEFINE_CLASS_VISITOR(SigmoidNode)
DEFINE_CLASS_VISITOR(TanhNode)
DEFINE_CLASS_VISITOR(ReturnNode)

void ConvolutionNode::visit(Node *parent, NodeVisitor *visitor) {
  if (!visitor->shouldVisit(parent, this)) {
    return;
  }
  visitor->pre(parent, this);
  in_->visit(this, visitor);
  filter_->visit(this, visitor);
  bias_->visit(this, visitor);
  visitor->post(parent, this);
}

void FullyConnectedNode::visit(Node *parent, NodeVisitor *visitor) {
  if (!visitor->shouldVisit(parent, this)) {
    return;
  }
  visitor->pre(parent, this);
  in_->visit(this, visitor);
  filter_->visit(this, visitor);
  bias_->visit(this, visitor);
  visitor->post(parent, this);
}

void BatchNormalizationNode::visit(Node *parent, NodeVisitor *visitor) {
  if (!visitor->shouldVisit(parent, this)) {
    return;
  }
  visitor->pre(parent, this);
  in_->visit(this, visitor);
  scale_->visit(this, visitor);
  bias_->visit(this, visitor);
  mean_->visit(this, visitor);
  var_->visit(this, visitor);
  visitor->post(parent, this);
}

void ArithmeticNode::visit(Node *parent, NodeVisitor *visitor) {
  if (!visitor->shouldVisit(parent, this)) {
    return;
  }
  visitor->pre(parent, this);
  LHS_->visit(this, visitor);
  RHS_->visit(this, visitor);
  visitor->post(parent, this);
}

void SoftMaxNode::visit(Node *parent, NodeVisitor *visitor) {
  if (!visitor->shouldVisit(parent, this)) {
    return;
  }
  visitor->pre(parent, this);
  in_->visit(this, visitor);
  selected_->visit(this, visitor);
  visitor->post(parent, this);
}

void RegressionNode::visit(Node *parent, NodeVisitor *visitor) {
  if (!visitor->shouldVisit(parent, this)) {
    return;
  }
  visitor->pre(parent, this);
  in_->visit(this, visitor);
  expected_->visit(this, visitor);
  visitor->post(parent, this);
}

void ConcatNode::visit(Node *parent, NodeVisitor *visitor) {
  if (!visitor->shouldVisit(parent, this)) {
    return;
  }
  visitor->pre(parent, this);
  for (auto &I : in_) {
    I->visit(this, visitor);
  }
  visitor->post(parent, this);
}

//===----------------------------------------------------------------------===//
//                     Debug description methods
//===----------------------------------------------------------------------===//
std::string Node::getDebugDesc() const { return "<node>"; }

std::string Variable::getDebugDesc() const {
  DescriptionBuilder db(getKindName());
  db.addParam("name", quote(getName()))
      .addParam("output", *getType())
  .addParam("init", WeightVar::getInitKindStr(initKind_));
  if (initKind_ != Variable::InitKind::Extern) {
    db.addParam("val", val_);
  }
  db.addParam("users", getNumUsers());
  return db;
}

std::string ConvolutionNode::getDebugDesc() const {
  DescriptionBuilder db(getKindName());
  db.addParam("name", quote(getName()))
      .addParam("input", *in_->getType())
      .addParam("output", *getType())
      .addParam("filter", *filter_->getType())
      .addParam("bias", *bias_->getType())
      .addParam("kernel", kernel_)
      .addParam("stride", stride_)
      .addParam("pad", pad_)
      .addParam("depth", depth_)
      .addParam("users", getNumUsers());
  return db;
}
std::string PoolNode::getDebugDesc() const {
  DescriptionBuilder db(getKindName());
  db.addParam("name", quote(getName()))
      .addParam("input", *in_->getType())
      .addParam("output", *getType())
      .addParam("kernel", kernel_)
      .addParam("stride", stride_)
      .addParam("pad", pad_)
      .addParam("kind", kind_ == OpKind::Max ? "max" : "avg")
      .addParam("users", getNumUsers());
  return db;
}

std::string FullyConnectedNode::getDebugDesc() const {
  DescriptionBuilder db(getKindName());
  db.addParam("name", quote(getName()))
      .addParam("input", *in_->getType())
      .addParam("output", *getType())
      .addParam("filter", *filter_->getType())
      .addParam("bias", *bias_->getType())
      .addParam("depth", depth_)
      .addParam("users", getNumUsers());
  return db;
}

std::string LocalResponseNormalizationNode::getDebugDesc() const {
  DescriptionBuilder db(getKindName());
  db.addParam("name", quote(getName()))
      .addParam("input", *in_->getType())
      .addParam("alpha", alpha_)
      .addParam("beta", beta_)
      .addParam("half window size", this->halfWindowSize_)
      .addParam("scale", *scale_->getType())
      .addParam("users", getNumUsers());
  return db;
}

std::string ConcatNode::getDebugDesc() const {
  DescriptionBuilder db(getKindName());
  db.addParam("name", quote(getName()));

  for (auto &input : in_) {
    db.addParam("input", *input->getType());
  }
  db.addParam("output", *getType())
      .addParam("dimension", dim_)
      .addParam("users", getNumUsers());
  return db;
}

std::string SoftMaxNode::getDebugDesc() const {
  DescriptionBuilder db(getKindName());
  db.addParam("name", quote(getName()))
      .addParam("input", *in_->getType())
      .addParam("selected", *selected_->getType())
      .addParam("users", getNumUsers());
  return db;
}

std::string RegressionNode::getDebugDesc() const {
  DescriptionBuilder db(getKindName());
  db.addParam("name", quote(getName()))
      .addParam("input", *in_->getType())
      .addParam("expected", *expected_->getType())
      .addParam("users", getNumUsers());
  return db;
}

std::string BatchNormalizationNode::getDebugDesc() const {
  DescriptionBuilder db(getKindName());
  db.addParam("name", quote(getName()))
      .addParam("input", *in_->getType())
      .addParam("beta", *bias_->getType())
      .addParam("gamma", *scale_->getType())
      .addParam("channelIdx", channelIdx_)
      .addParam("epsilon", epsilon_)
      .addParam("momentum", momentum_)
      .addParam("users", getNumUsers());
  return db;
}

std::string ArithmeticNode::getDebugDesc() const {
  DescriptionBuilder db(getKindName());
  db.addParam("name", quote(getName()))
      .addParam("output", *getType())
      .addParam("op", kind_ == ArithmeticInst::OpKind::Add ? "add" : "mul")
      .addParam("users", getNumUsers());
  return db;
}

std::string ReturnNode::getDebugDesc() const {
  DescriptionBuilder db(getKindName());
  db.addParam("name", quote(getName())).addParam("users", getNumUsers());
  return db;
}

std::string TransposeNode::getDebugDesc() const {
  std::string sh = arrayRefToString(llvm::ArrayRef<unsigned>(shuffle_));
  DescriptionBuilder db(getKindName());
  db.addParam("name", quote(getName()))
      .addParam("shuffle", sh)
      .addParam("users", getNumUsers());
  return db;
}

#define DEFINE_CLASS_REPR(CLASS_NAME)                                          \
  std::string CLASS_NAME::getDebugDesc() const {                               \
    DescriptionBuilder db(getKindName());                                      \
    db.addParam("name", quote(getName()))                                      \
        .addParam("input", *in_->getType())                                    \
        .addParam("users", getNumUsers());                                     \
    return db;                                                                 \
  }

DEFINE_CLASS_REPR(ReluNode);
DEFINE_CLASS_REPR(ReshapeNode);
DEFINE_CLASS_REPR(SigmoidNode);
DEFINE_CLASS_REPR(TanhNode);
