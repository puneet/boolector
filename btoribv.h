/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2012-2014 Armin Biere.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#ifndef BTORIBV_H_INCLUDED
#define BTORIBV_H_INCLUDED

#include "BitVector.h"

extern "C" {
#include "btorabort.h"
#include "btorcore.h"
#include "btormc.h"
#include "utils/btorstack.h"
};

// We use classical C style data structures in order to be able to use the
// Boolector memory manager which is hard to use for C++ allocators ('new'
// and 'delete').  This needs explicit 'tags' and 'unions'.

enum BtorIBVTag
{

  BTOR_IBV_IS_UNARY    = 16,
  BTOR_IBV_BUF         = 16 + 1,
  BTOR_IBV_NOT         = 16 + 2,
  BTOR_IBV_ZERO_EXTEND = 16 + 3,
  BTOR_IBV_SIGN_EXTEND = 16 + 4,
  BTOR_IBV_REPLICATE   = 16 + 5,
  BTOR_IBV_NON_STATE   = 16 + 6,
  BTOR_IBV_MAX_UNARY   = BTOR_IBV_NON_STATE,

  BTOR_IBV_IS_BINARY   = 32,
  BTOR_IBV_OR          = 32 + 1,
  BTOR_IBV_AND         = 32 + 2,
  BTOR_IBV_XOR         = 32 + 3,
  BTOR_IBV_LT          = 32 + 4,
  BTOR_IBV_LE          = 32 + 5,
  BTOR_IBV_SUM         = 32 + 6,
  BTOR_IBV_SUB         = 32 + 7,
  BTOR_IBV_MUL         = 32 + 8,
  BTOR_IBV_DIV         = 32 + 9,
  BTOR_IBV_MOD         = 32 + 10,
  BTOR_IBV_LEFT_SHIFT  = 32 + 11,
  BTOR_IBV_RIGHT_SHIFT = 32 + 12,
  BTOR_IBV_EQUAL       = 32 + 13,
  BTOR_IBV_STATE       = 32 + 14,
  BTOR_IBV_MAX_BINARY  = BTOR_IBV_STATE,

  BTOR_IBV_IS_TERNARY  = 64,
  BTOR_IBV_COND        = 64 + 1,
  BTOR_IBV_CONDBW      = 64 + 2,
  BTOR_IBV_MAX_TERNARY = BTOR_IBV_CONDBW,

  BTOR_IBV_IS_VARIADIC  = 128,
  BTOR_IBV_CONCAT       = 128 + 1,
  BTOR_IBV_CASE         = 128 + 2,
  BTOR_IBV_PARCASE      = 128 + 3,
  BTOR_IBV_MAX_VARIADIX = BTOR_IBV_PARCASE,

  BTOR_IBV_IS_PREDICATE = 256,
  BTOR_IBV_HAS_ARG      = 512,

  BTOR_IBV_OPS   = 255,
  BTOR_IBV_FLAGS = 16 | 32 | 64 | 128 | 256 | 512,
};

struct BtorIBVRange
{
  unsigned id, msb, lsb;
  BtorIBVRange (unsigned i, unsigned m, unsigned l) : id (i), msb (m), lsb (l)
  {
  }
  BtorIBVRange (const BitVector::BitRange &r);
  unsigned getWidth () const { return msb - lsb + 1; }
};

extern "C" {
BTOR_DECLARE_STACK (BtorIBVRange, BtorIBVRange);
};

struct BtorIBVAssignment
{
  BtorIBVTag tag;
  BtorIBVRange range;
  unsigned arg, nranges;
  BtorIBVRange *ranges;
  BtorIBVAssignment (BtorIBVTag t,
                     BtorIBVRange r,
                     unsigned a,
                     unsigned n       = 0,
                     BtorIBVRange *rs = 0)
      : tag (t), range (r), arg (a), nranges (n), ranges (rs)
  {
  }
};

extern "C" {
BTOR_DECLARE_STACK (BtorIBVAssignment, BtorIBVAssignment);
};

struct BtorIBVExpPushed
{
  BoolectorNode *exp;
  long pushed;
  BtorIBVExpPushed () : exp (0), pushed (0) {}
};

struct BtorIBVAtom
{
  BtorIBVRange range;
  BtorIBVExpPushed current, next;
  BtorIBVAtom (const BtorIBVRange &r) : range (r) {}
};

extern "C" {
BTOR_DECLARE_STACK (BtorIBVAtom, BtorIBVAtom);
};

struct BtorIBVRangeName
{
  struct
  {
    unsigned msb, lsb;
  } from, to;
  char *name;
};

extern "C" {
BTOR_DECLARE_STACK (BtorIBVRangeName, BtorIBVRangeName);
};

struct BtorIBVNode;

struct BtorIBVAtomPtrNext
{
  BtorIBVAtom *atom;
  bool next;
  BtorIBVAtomPtrNext (BtorIBVAtom *a, bool x) : atom (a), next (x) {}
};

extern "C" {
BTOR_DECLARE_STACK (BtorIBVAtomPtrNext, BtorIBVAtomPtrNext);
};

enum BtorIBVClassification
{
  BTOR_IBV_UNCLASSIFIED = 0,
  BTOR_IBV_CONSTANT,
  BTOR_IBV_ASSIGNED,
  BTOR_IBV_ASSIGNED_IMPLICIT_CURRENT,
  BTOR_IBV_ASSIGNED_IMPLICIT_NEXT,
  BTOR_IBV_CURRENT_STATE,
  BTOR_IBV_TWO_PHASE_INPUT,
  BTOR_IBV_ONE_PHASE_ONLY_CURRENT_INPUT,
  BTOR_IBV_ONE_PHASE_ONLY_NEXT_INPUT,
  BTOR_IBV_PHANTOM_CURRENT_INPUT,
  BTOR_IBV_PHANTOM_NEXT_INPUT,
  BTOR_IBV_NOT_USED,
};

struct BtorIBVFlags
{
  BtorIBVClassification classified;
  bool assigned, used, coi, input, onephase, forwarded;
  struct
  {
    bool current, next;
    unsigned mark : 2;
  } depends;
  struct
  {
    bool current, next;
  } state, nonstate, implicit;
};

struct BtorIBVNode
{
  unsigned width;
  unsigned id;
  bool is_constant;
  bool is_next_state;
  BitVector::BvVariableSource source;
  BitVector::DirectionKind direction;
  signed char marked, used, coi;
  BoolectorNode *cached;
  char *name;
  BtorIBVFlags *flags;
  BtorIBVAssignment **assigned;
  BtorIBVAssignment **next, **prev;
  BtorIBVAssignmentStack assignments;
  BtorIBVRangeNameStack ranges;
  BtorIBVAtomStack atoms;
};

extern "C" {
BTOR_DECLARE_STACK (BtorIBVNodePtr, BtorIBVNode *);
};

struct BtorIBVBit
{
  unsigned id, bit;
  BtorIBVBit (unsigned i, unsigned b) : id (i), bit (b) {}
  BtorIBVBit (const BitVector::Bit &b);
};

typedef struct BtorIBVBit BtorIBVBit;

extern "C" {
BTOR_DECLARE_STACK (BtorIBVBit, BtorIBVBit);
};

struct BtorIBVAssumption
{
  BtorIBVRange range;
  bool initial;
  BtorIBVAssumption (const BtorIBVRange &r, bool i) : range (r), initial (i) {}
};

extern "C" {
BTOR_DECLARE_STACK (BtorIBVAssumption, BtorIBVAssumption);
};

class BtorIBV : public BitVector
{
  enum State
  {
    BTOR_IBV_START,
    BTOR_IBV_ANALYZED,
    BTOR_IBV_TRANSLATED,
  } state;

  Btor *btor;
  BtorMC *btormc;

  bool gentrace;
  int force;
  int verbosity;

  BtorIBVNodePtrStack idtab;
  BtorIBVBitStack assertions;
  BtorIBVAssumptionStack assumptions;

  //------------------------------------------------------------------------

  BtorIBVNode *id2node (unsigned id)
  {
    BtorIBVNode *node;
    assert (0 < id);
    node = BTOR_PEEK_STACK (idtab, id);
    assert (node);
    return node;
  }

  BtorIBVNode *bitrange2node (BitRange range)
  {
    assert (range.m_nLsb <= range.m_nMsb);
    BtorIBVNode *node = id2node (range.m_nId);
    assert (range.getWidth () <= node->width);
    assert (range.m_nMsb < node->width);
    return node;
  }

  void check_bit_range (BitRange range) { (void) bitrange2node (range); }

  BtorIBVNode *new_node (unsigned id, unsigned width);

  bool mark_coi (BtorIBVNode *, unsigned);
  bool mark_used (BtorIBVNode *, unsigned);
  void mark_assigned (BtorIBVNode *, BitRange);

  void mark_current_state (BtorIBVNode *, BitRange);
  void mark_current_nonstate (BtorIBVNode *, BitRange);
  void mark_next_state (BtorIBVNode *, BitRange);
  void mark_next_nonstate (BtorIBVNode *, BitRange);

  void delete_ibv_release_variable (BtorIBVNode *);
  void delete_ibv_node (BtorIBVNode *);

  //------------------------------------------------------------------------

  void addUnary (BtorIBVTag, BitRange, BitRange);

  void addUnaryOp (BtorIBVTag tag, BitRange o, BitRange a)
  {
    assert (tag & BTOR_IBV_IS_UNARY);
    assert (tag <= BTOR_IBV_MAX_UNARY);
    addUnary (tag, o, a);
  }

  void addUnaryArg (BtorIBVTag, BitRange, BitRange, unsigned);

  void addUnaryPred (BtorIBVTag tag, BitRange o, BitRange a)
  {
    assert (o.getWidth () == 1);
    assert (tag & BTOR_IBV_IS_UNARY);
    assert (tag <= BTOR_IBV_MAX_UNARY);
    if (a.getWidth () != 1) tag = (BtorIBVTag) (tag | BTOR_IBV_IS_PREDICATE);
    addUnary (tag, o, a);
  }

  //------------------------------------------------------------------------

  void addBinary (BtorIBVTag, BitRange, BitRange, BitRange);

  void addBinaryOp (BtorIBVTag tag, BitRange o, BitRange a, BitRange b)
  {
    assert (tag & BTOR_IBV_IS_BINARY);
    assert (tag <= BTOR_IBV_MAX_BINARY);
    addBinary (tag, o, a, b);
  }

  void addBinaryPred (BtorIBVTag tag, BitRange o, BitRange a, BitRange b)
  {
    assert (o.getWidth () == 1);
    assert (tag & BTOR_IBV_IS_BINARY);
    assert (tag <= BTOR_IBV_MAX_BINARY);
    assert (a.getWidth () == b.getWidth ());
    if (a.getWidth () != 1) tag = (BtorIBVTag) (tag | BTOR_IBV_IS_PREDICATE);
    addBinary (tag, o, a, b);
  }

  //------------------------------------------------------------------------

  void addCaseOp (BtorIBVTag tag, BitRange o, const vector<BitRange> &ops);

  //------------------------------------------------------------------------

  void wrn (const char *fmt, ...);

  void print (const BtorIBVAssignment &);    // to 'stdout' without NL
  void println (const BtorIBVAssignment &);  // to 'stdout' with NL
  void printf3 (const char *fmt, ...);

  void msg (int level, const char *fmt, ...);
  void msg (int level, const BtorIBVAssignment &, const char *, ...);

  void warn (const char *fmt, ...);

  bool is_relevant_atom_for_assigned_atom (BtorIBVAtom *lhs,
                                           unsigned i,
                                           BtorIBVAtom *rhs,
                                           BtorIBVAssignment *);

  void push_atom_ptr_next (BtorIBVAtom *,
                           bool forward,
                           BtorIBVAtomPtrNextStack *apnwork);

  void translate_atom_divide (BtorIBVAtom *, bool, BtorIBVAtomPtrNextStack *);
  bool translate_atom_conquer (BtorIBVAtom *, bool);

  BoolectorNode *translate_assignment_conquer (BtorIBVAtom *,
                                               bool,
                                               BtorIBVAssignment *);

  void translate_atom_base (BtorIBVAtom *);

  bool is_phantom_current (BtorIBVNode *, unsigned);
  bool is_phantom_next (BtorIBVNode *, unsigned);

  struct
  {
    unsigned inputs, latches, nexts, inits, bads, constraints;
  } stats;

 public:
  class ReachedAtBoundListener
  {
   public:
    ReachedAtBoundListener () {}
    virtual ~ReachedAtBoundListener () {}
    virtual void reachedAtBound (int assertion_number, int k) = 0;
  };

  BtorIBV ();
  ~BtorIBV ();

  void setRewriteLevel (int rwl);
  void setForce (int f = 1) { force = f; }

  void setVerbosity (int verbosity);

  void enableTraceGeneration ();

  //------------------------------------------------------------------------
  // Default is to stop at the first reached bad state property.  Given a
  // false to this function will result in the model checker to run until
  // all properties have been reached (or proven not to be reachable) or the
  // maximum bound is reached.
  //
  void setStop (bool stop);

  // First alternative C-style function pointer API (as in 'btormc.h').
  //
  void setReachedAtBoundCallBack (void *state,
                                  void (*fun) (void *state, int i, int k));

  // Second C++ Listener API.
  //
  void setReachedAtBoundListener (ReachedAtBoundListener *);

  // Return the 'k' at which a previous model checking run showed that the
  // assertion with number 'assertion_number' (counting from 0) has been
  // violated.  It returns a negative number if the property was not violated
  // during the last BMC run.
  //
  int hasAssertionBeenViolatedAtBound (int assertion_number);

  // TODO do we need BitRange instead of 'assertion_number' both for
  // 'hasAssertionBeenViolatedAtBound' and the listener?

  //------------------------------------------------------------------------

#define BTOR_IBV_REQUIRE_START()                                          \
  do                                                                      \
  {                                                                       \
    BTOR_ABORT_BOOLECTOR (state == BTOR_IBV_ANALYZED,                     \
                          "can not change model it has been analyzed");   \
    BTOR_ABORT_BOOLECTOR (state == BTOR_IBV_TRANSLATED,                   \
                          "can not change model it has been translated"); \
    assert (state == BTOR_IBV_START);                                     \
  } while (0)

  void addConstant (unsigned, const string &, unsigned);

  void addVariable (unsigned,
                    const string &,
                    unsigned,
                    bool,
                    BvVariableSource,
                    DirectionKind);

  void addRangeName (BitRange, const string &, unsigned, unsigned);

  //------------------------------------------------------------------------

  void addState (BitRange, BitRange, BitRange);

  void addNonState (BitRange, BitRange);

  //------------------------------------------------------------------------

  void addAssignment (BitRange o, BitRange a)
  {
    BTOR_IBV_REQUIRE_START ();
    addUnaryOp (BTOR_IBV_BUF, o, a);
  }
  void addBitNot (BitRange o, BitRange a)
  {
    BTOR_IBV_REQUIRE_START ();
    addUnaryOp (BTOR_IBV_NOT, o, a);
  }
  void addZeroExtension (BitRange o, BitRange a)
  {
    BTOR_IBV_REQUIRE_START ();
    addUnaryOp (BTOR_IBV_ZERO_EXTEND, o, a);
  }
  void addSignExtension (BitRange o, BitRange a)
  {
    BTOR_IBV_REQUIRE_START ();
    addUnaryOp (BTOR_IBV_SIGN_EXTEND, o, a);
  }
  void addLogicalNot (BitRange o, BitRange a)
  {
    BTOR_IBV_REQUIRE_START ();
    addUnaryPred (BTOR_IBV_NOT, o, a);
  }

  //------------------------------------------------------------------------

  void addBitOr (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryOp (BTOR_IBV_OR, o, a, b);
  }
  void addBitAnd (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryOp (BTOR_IBV_AND, o, a, b);
  }
  void addBitXor (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryOp (BTOR_IBV_XOR, o, a, b);
  }
  void addEqual (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryPred (BTOR_IBV_EQUAL, o, a, b);
  }
  void addGreaterThan (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryPred (BTOR_IBV_LT, o, b, a);
  }
  void addGreaterEqual (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryPred (BTOR_IBV_LE, o, b, a);
  }
  void addLessThan (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryPred (BTOR_IBV_LT, o, a, b);
  }
  void addLessEqual (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryPred (BTOR_IBV_LE, o, a, b);
  }
  void addLogicalAnd (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryPred (BTOR_IBV_AND, o, a, b);
  }
  void addLogicalOr (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryPred (BTOR_IBV_OR, o, a, b);
  }
  void addSum (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryOp (BTOR_IBV_SUM, o, a, b);
  }
  void addSub (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryOp (BTOR_IBV_SUB, o, a, b);
  }
  void addMul (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryOp (BTOR_IBV_MUL, o, a, b);
  }
  void addDiv (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryOp (BTOR_IBV_DIV, o, a, b);
  }
  void addMod (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryOp (BTOR_IBV_MOD, o, a, b);
  }
  void addLShiftNonConst (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryOp (BTOR_IBV_LEFT_SHIFT, o, a, b);
  }
  void addRShiftNonConst (BitRange o, BitRange a, BitRange b)
  {
    BTOR_IBV_REQUIRE_START ();
    addBinaryOp (BTOR_IBV_RIGHT_SHIFT, o, a, b);
  }

  //------------------------------------------------------------------------

  void addCondition (BitRange, BitRange, BitRange, BitRange);

  //------------------------------------------------------------------------

  void addReplicate (BitRange o, BitRange a, unsigned arg)
  {
    BTOR_IBV_REQUIRE_START ();
    addUnaryArg (BTOR_IBV_REPLICATE, o, a, arg);
  }
  void addLShift (BitRange o, BitRange a, unsigned arg)
  {
    BTOR_IBV_REQUIRE_START ();
    addUnaryArg (BTOR_IBV_LEFT_SHIFT, o, a, arg);
  }
  void addRShift (BitRange o, BitRange a, unsigned arg)
  {
    BTOR_IBV_REQUIRE_START ();
    addUnaryArg (BTOR_IBV_RIGHT_SHIFT, o, a, arg);
  }

  //------------------------------------------------------------------------

  void addConcat (BitRange output, const vector<BitRange> &operands);

  void addCase (BitRange o, const vector<BitRange> &ops)
  {
    BTOR_IBV_REQUIRE_START ();
    addCaseOp (BTOR_IBV_CASE, o, ops);
  }

  void addParallelCase (BitRange o, const vector<BitRange> &ops)
  {
    BTOR_IBV_REQUIRE_START ();
    addCaseOp (BTOR_IBV_PARCASE, o, ops);
  }

  //------------------------------------------------------------------------

  void addAssertion (BitRange);
  void addAssumption (BitRange, bool);

#if 0
  void addFairnessConstraint (BitRange, BitRange);

  //------------------------------------------------------------------------

  void addMemory (unsigned, const string&,
                  unsigned, unsigned,  unsigned, unsigned,
                  const vector<string>&);
  void addMemoryRead (unsigned, BitRange, unsigned, unsigned, BitRange);
  void addMemoryWrite (unsigned, unsigned, BitRange,
                       unsigned, unsigned, BitRange, BitRange);
  void addMemoryConstantWrite (unsigned, unsigned, unsigned, unsigned,
                               unsigned, unsigned, BitRange, BitRange);
  void addMemoryEqual (BitRange output,
		       unsigned, unsigned, unsigned, unsigned,
		       unsigned, unsigned, unsigned, unsigned,
		       unsigned, unsigned, bool);
#endif

  //------------------------------------------------------------------------

  void analyze ();    // Needs to be called before 'translate'.
  void translate ();  // Into internal BtorMC model.

  void dump_btor (FILE *file);  // Dump BTOR model to this file.

  int bmc (int mink, int maxk);
  string assignment (BitRange, int k);
};

inline BtorIBVRange::BtorIBVRange (const BitVector::BitRange &r)
    : id (r.m_nId), msb (r.m_nMsb), lsb (r.m_nLsb)
{
}

inline BtorIBVBit::BtorIBVBit (const BitVector::Bit &b)
    : id (b.m_nId), bit (b.m_nBit)
{
}

#endif
