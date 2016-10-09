/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2016 Mathias Preiner.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#ifndef BTORSYNTHFUN_H_INCLUDED
#define BTORSYNTHFUN_H_INCLUDED

#include <stdint.h>
#include "btorexp.h"
#include "btortypes.h"
#include "utils/btorhashptr.h"

enum BtorSynthType
{
  BTOR_SYNTH_TYPE_NONE,
  BTOR_SYNTH_TYPE_SK_VAR,
  BTOR_SYNTH_TYPE_SK_UF,
  BTOR_SYNTH_TYPE_UF,
};

typedef enum BtorSynthType BtorSynthType;

BtorNode* btor_synthesize_fun (Btor* btor,
                               const BtorPtrHashTable* model,
                               BtorNode* prev_synth_fun,
                               BtorPtrHashTable* additional_inputs,
                               BtorNode* consts[],
                               uint32_t nconsts,
                               uint32_t max_num_checks,
                               uint32_t max_level);

BtorNode* btor_synthesize_fun_constraints (Btor* btor,
                                           const BtorPtrHashTable* model,
                                           BtorNode* prev_synth_fun,
                                           BtorNode* var,
                                           BtorNode* constraints[],
                                           uint32_t nconstraints,
                                           BtorNode* cinputs[],
                                           uint32_t ncinputs,
                                           BtorNode* consts[],
                                           uint32_t nconsts,
                                           uint32_t max_num_checks,
                                           uint32_t max_level);
#endif
