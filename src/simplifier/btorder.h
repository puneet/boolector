/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2016 Mathias Preiner.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#ifndef BTORDER_H_INCLUDED
#define BTORDER_H_INCLUDED

#include "btortypes.h"

// void btor_der (Btor * btor);

BtorNode* btor_der_node (Btor* btor, BtorNode* root);

BtorNode* btor_cer_node (Btor* btor, BtorNode* root);

#endif