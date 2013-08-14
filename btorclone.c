#include "btoraig.h"
#include "btoraigvec.h"
#include "btorexp.h"
#include "btorhash.h"
#include "btormap.h"
#include "btorstack.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

BTOR_DECLARE_STACK (NodePtrPtr, BtorNode **);
BTOR_DECLARE_STACK (AIGPtrPtr, BtorAIG **);

static BtorAIG *
clone_aig (Btor *btor,
           Btor *clone,
           BtorAIG *aig,
           BtorAIGPtrPtrStack *aigs,
           BtorAIGMap *aig_map)
{
  assert (aig);
  assert (aig_map);

  int i;
  BtorAIG *res, *tmp, *real_aig;
  BtorMemMgr *mm, *cmm;

  mm  = btor->mm;
  cmm = clone->mm;

  real_aig = BTOR_REAL_ADDR_AIG (aig);

  BTOR_NEWN (cmm, res, 1);
  memcpy (real_aig, res, sizeof *real_aig);

  for (i = 0; i < 2 && real_aig->children[i]; i++)
  {
    tmp = btor_mapped_aig (aig_map, real_aig->children[i]);
    assert (tmp);
    res->children[i] = tmp;
  }

  BTOR_PUSH_STACK (mm, *aigs, &real_aig->next);

  return BTOR_IS_INVERTED_AIG (aig) ? BTOR_INVERT_AIG (res) : res;
}

static BtorAIGVec *
clone_av (Btor *btor,
          Btor *clone,
          BtorAIGVec *av,
          BtorAIGPtrPtrStack *aigs,
          BtorAIGMap *aig_map)
{
  int i;
  BtorAIG *cur, *real_cur;
  BtorAIGVec *res;
  BtorAIGPtrStack stack, unmark_stack;
  BtorMemMgr *mm, *cmm;

  mm  = btor->mm;
  cmm = clone->mm;

  BTOR_INIT_STACK (stack);
  BTOR_INIT_STACK (unmark_stack);

  BTOR_NEWN (cmm, res, 1);
  memcpy (av, res, sizeof *av);

  for (i = 0; i < av->len; i++)
  {
    BTOR_PUSH_STACK (mm, stack, av->aigs[i]);
    while (!BTOR_EMPTY_STACK (stack))
    {
      cur      = BTOR_POP_STACK (stack);
      real_cur = BTOR_REAL_ADDR_AIG (cur);

      if (real_cur->clone_mark >= 2 || btor_mapped_aig (aig_map, cur)) continue;

      if (real_cur->clone_mark == 0)
      {
        real_cur->clone_mark = 1;
        BTOR_PUSH_STACK (mm, stack, cur);
        BTOR_PUSH_STACK (mm, unmark_stack, real_cur);

        BTOR_PUSH_STACK (mm, stack, real_cur->children[0]);
        BTOR_PUSH_STACK (mm, stack, real_cur->children[1]);
      }
      else
      {
        assert (real_cur->clone_mark == 1);
        real_cur->clone_mark = 2;
        btor_map_aig (
            btor, aig_map, cur, clone_aig (btor, clone, cur, aigs, aig_map));
      }
    }
    res->aigs[i] = btor_mapped_aig (aig_map, av->aigs[i]);
    assert (res->aigs[i]);
  }

  /* reset mark flags */
  while (!BTOR_EMPTY_STACK (unmark_stack))
  {
    real_cur = BTOR_POP_STACK (unmark_stack);
    assert (real_cur->clone_mark == 2);
    real_cur->clone_mark = 0;
  }

  return res;
}

static void
clone_exp (Btor *btor,
           Btor *clone,
           BtorNode *exp,
           BtorNodePtrPtrStack *nodes,
           BtorAIGPtrPtrStack *aigs,
           BtorNodeMap *exp_map,
           BtorAIGMap *aig_map)
{
  assert (btor);
  assert (clone);
  assert (exp);
  assert (nodes);
  assert (exp_map);
  assert (aig_map);

  int i, len;
  BtorNode *res, *real_exp;
  BtorMemMgr *mm, *cmm;

  mm  = btor->mm;
  cmm = clone->mm;

  real_exp = BTOR_REAL_ADDR_NODE (exp);
  assert (!BTOR_IS_PROXY_NODE (real_exp));

  res = btor_malloc (cmm, real_exp->bytes);
  memcpy (real_exp, res, sizeof *real_exp);

  if (real_exp->bits)
  {
    len = strlen (real_exp->bits);
    BTOR_NEWN (cmm, res->bits, len);
    for (i = 0; i < len; i++) res->bits[i] = real_exp->bits[i];
  }

  /* Note: no need to cache aig vectors here (exp->av is unique to exp). */
  if (!BTOR_IS_ARRAY_NODE (real_exp) && real_exp->av)
    res->av = clone_av (btor, clone, real_exp->av, aigs, aig_map);
  /* else: no need to clone rho (valid only during consistency checking) */

  BTOR_PUSH_STACK (mm, *nodes, &real_exp->next);
  BTOR_PUSH_STACK (mm, *nodes, &real_exp->parent);

  assert (!real_exp->simplified);

  res->btor = clone;

  BTOR_PUSH_STACK (mm, *nodes, &real_exp->first_parent);
  BTOR_PUSH_STACK (mm, *nodes, &real_exp->last_parent);

  if (!BTOR_IS_BV_CONST_NODE (real_exp) && !BTOR_IS_BV_VAR_NODE (real_exp)
      && !BTOR_IS_ARRAY_VAR_NODE (real_exp) && !BTOR_IS_PARAM_NODE (real_exp))
  {
    if (real_exp->arity)
    {
      for (i = 0; i < real_exp->arity; i++)
      {
        res->e[i] = btor_mapped_node (exp_map, real_exp->e[i]);
        assert (res->e[i]);
      }
    }
    else
    {
      res->symbol = btor_strdup (cmm, real_exp->symbol);
      if (BTOR_IS_ARRAY_EQ_NODE (real_exp))
      {
        assert (btor_mapped_node (exp_map, real_exp->vreads->exp1));
        assert (btor_mapped_node (exp_map, real_exp->vreads->exp2));
        res->vreads =
            new_exp_pair (clone,
                          btor_mapped_node (exp_map, real_exp->vreads->exp1),
                          btor_mapped_node (exp_map, real_exp->vreads->exp2));
      }
    }

    for (i = 0; i < real_exp->arity; i++)
    {
      BTOR_PUSH_STACK (mm, *nodes, &real_exp->prev_parent[i]);
      BTOR_PUSH_STACK (mm, *nodes, &real_exp->next_parent[i]);
    }
  }

  if (BTOR_IS_ARRAY_NODE (real_exp))
  {
    BTOR_PUSH_STACK (mm, *nodes, &real_exp->first_aeq_acond_parent);
    BTOR_PUSH_STACK (mm, *nodes, &real_exp->last_aeq_acond_parent);

    if (!BTOR_IS_ARRAY_VAR_NODE (real_exp))
    {
      for (i = 0; i < real_exp->arity; i++)
      {
        BTOR_PUSH_STACK (mm, *nodes, &real_exp->prev_aeq_acond_parent[i]);
        BTOR_PUSH_STACK (mm, *nodes, &real_exp->next_aeq_acond_parent[i]);
      }
    }
  }

  res = BTOR_IS_INVERTED_NODE (exp) ? BTOR_INVERT_NODE (res) : res;
  btor_map_node (btor, exp_map, exp, res);
}

static void
clone_constraints (Btor *btor,
                   Btor *clone,
                   BtorNodeMap *exp_map,
                   BtorAIGMap *aig_map)
{
  assert (btor);
  assert (clone);
  assert (exp_map);
  assert (aig_map);

  int i;
  BtorNode *cur, *real_cur, *cloned_exp, **parent, *real_parent;
  BtorNodePtrStack stack, unmark_stack;
  BtorNodePtrPtrStack nodes;
  BtorAIG **next, *cloned_aig;
  BtorAIGPtrPtrStack aigs;
  BtorPtrHashBucket *b;
  BtorPtrHashTable **c, **r;
  BtorMemMgr *mm;

  BtorPtrHashTable *constraints[]     = {btor->unsynthesized_constraints,
                                     btor->synthesized_constraints,
                                     btor->embedded_constraints,
                                     NULL},
                   *res_constraints[] = {clone->unsynthesized_constraints,
                                         clone->synthesized_constraints,
                                         clone->embedded_constraints,
                                         NULL};

  BTOR_INIT_STACK (stack);
  BTOR_INIT_STACK (unmark_stack);
  BTOR_INIT_STACK (nodes);
  BTOR_INIT_STACK (aigs);

  mm = btor->mm;

  for (c = constraints, r = res_constraints; *c; c++, r++)
  {
    for (b = (*c)->first; b; b = b->next)
    {
      cur = (BtorNode *) b->key;
      BTOR_PUSH_STACK (mm, stack, cur);
      while (!BTOR_EMPTY_STACK (stack))
      {
        cur      = BTOR_POP_STACK (stack);
        real_cur = BTOR_REAL_ADDR_NODE (cur);

        if (real_cur->clone_mark >= 2) continue;

        if (real_cur->clone_mark == 0)
        {
          real_cur->clone_mark = 1;
          BTOR_PUSH_STACK (mm, stack, cur);
          BTOR_PUSH_STACK (mm, unmark_stack, real_cur);

          if (BTOR_IS_ARRAY_EQ_NODE (real_cur))
          {
            BTOR_PUSH_STACK (mm, stack, real_cur->vreads->exp1);
            BTOR_PUSH_STACK (mm, stack, real_cur->vreads->exp2);
          }

          for (i = 0; i < real_cur->arity; i++)
            BTOR_PUSH_STACK (mm, stack, real_cur->e[i]);
        }
        else
        {
          assert (real_cur->clone_mark == 1);
          real_cur->clone_mark = 2;
          clone_exp (btor, clone, cur, &nodes, &aigs, exp_map, aig_map);
        }
      }

      cloned_exp = btor_mapped_node (exp_map, b->key);
      assert (cloned_exp);
      btor_insert_in_ptr_hash_table (*r, cloned_exp);
    }
  }

  /* clone true expression (special case) */
  clone_exp (btor, clone, btor->true_exp, &nodes, &aigs, exp_map, aig_map);

  /* update parent and next pointers of expressions */
  while (!BTOR_EMPTY_STACK (nodes))
  {
    parent      = BTOR_POP_STACK (nodes);
    real_parent = BTOR_REAL_ADDR_NODE (*parent);
    if (!real_parent) continue;
    cloned_exp = btor_mapped_node (exp_map, real_parent);
    assert (cloned_exp);
    cloned_exp = BTOR_TAG_NODE (cloned_exp, BTOR_GET_TAG_NODE (*parent));
    *parent    = cloned_exp;
  }

  /* update next pointers of aigs */
  while (!BTOR_EMPTY_STACK (aigs))
  {
    next = BTOR_POP_STACK (aigs);
    if (!*next) continue;
    cloned_aig = btor_mapped_aig (aig_map, *next);
    assert (cloned_aig);
    *next = cloned_aig;
  }

  /* reset mark flags */
  while (!BTOR_EMPTY_STACK (unmark_stack))
  {
    cur = BTOR_POP_STACK (unmark_stack);
    assert (cur->clone_mark == 2);
    cur->clone_mark = 0;
  }
}

static void
clone_ptr_hash_table (BtorPtrHashTable *table,
                      BtorPtrHashTable *res_table,
                      BtorNodeMap *exp_map)
{
  assert (table);
  assert (res_table);
  assert (exp_map);

  BtorNode *cloned_exp;
  BtorPtrHashBucket *b;

  for (b = table->first; b; b = b->next)
  {
    cloned_exp = btor_mapped_node (exp_map, b->key);
    assert (cloned_exp);
    btor_insert_in_ptr_hash_table (res_table, cloned_exp);
  }
}

static void
clone_node_ptr_stack (BtorMemMgr *mm,
                      BtorNodePtrStack *stack,
                      BtorNodePtrStack *res_stack,
                      BtorNodeMap *exp_map)
{
  assert (stack);
  assert (res_stack);
  assert (BTOR_EMPTY_STACK (*res_stack));
  assert (exp_map);

  int i;
  BtorNode *cloned_exp;

  for (i = 0; i < BTOR_COUNT_STACK (*stack); i++)
  {
    cloned_exp = btor_mapped_node (exp_map, (*stack).start[i]);
    assert (cloned_exp);
    BTOR_PUSH_STACK (mm, *res_stack, cloned_exp);
  }
}

Btor *
btor_clone_btor (Btor *btor)
{
  Btor *clone;
  BtorNodeMap *exp_map, *aig_map;

  exp_map = btor_new_node_map (btor);
  aig_map = btor_new_node_map (btor);

  clone = btor_new_btor ();

  memcpy (&clone->bv_lambda_id,
          &btor->bv_lambda_id,
          (char *) &btor->lod_cache - (char *) &btor->bv_lambda_id);
  memcpy (&clone->stats,
          &btor->stats,
          (char *) btor + sizeof (*btor) - (char *) &btor->stats);

  clone_constraints (btor, clone, exp_map, aig_map);

  clone_node_ptr_stack (
      clone->mm, &btor->nodes_id_table, &clone->nodes_id_table, exp_map);

  *clone->nodes_unique_table.chains =
      btor_mapped_node (exp_map, *btor->nodes_unique_table.chains);
  assert (*clone->nodes_unique_table.chains);

  // TODO sorts_unique_table (currently unused)

  clone_ptr_hash_table (btor->bv_vars, clone->bv_vars, exp_map);
  clone_ptr_hash_table (btor->array_vars, clone->array_vars, exp_map);
  clone_ptr_hash_table (btor->lambdas, clone->lambdas, exp_map);
  clone_ptr_hash_table (btor->substitutions, clone->substitutions, exp_map);

  clone->true_exp = btor_mapped_node (exp_map, btor->true_exp);
  assert (clone->true_exp);

  clone_ptr_hash_table (btor->lod_cache, clone->lod_cache, exp_map);
  clone_ptr_hash_table (btor->assumptions, clone->assumptions, exp_map);
  clone_ptr_hash_table (btor->var_rhs, clone->var_rhs, exp_map);
  clone_ptr_hash_table (btor->array_rhs, clone->array_rhs, exp_map);

  clone_node_ptr_stack (
      clone->mm, &btor->arrays_with_model, &clone->arrays_with_model, exp_map);

  clone_ptr_hash_table (btor->cache, clone->cache, exp_map);

  clone->clone         = NULL;
  clone->apitrace      = NULL;
  clone->closeapitrace = 0;

  return clone;
}
