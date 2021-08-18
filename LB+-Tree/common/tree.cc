/**
 * @file tree.cc
 * @author  Shimin Chen <shimin.chen@gmail.com>, Jihang Liu, Leying Chen
 * @version 1.0
 *
 * @section LICENSE
 *
 * TBD
 * 
 * @section DESCRIPTION
 *      
 * This file contains the main driver for experiments.
 */

#include "tree.h"

/* ------------------------------------------------------------------------ */
/*               global variables                                           */
/* ------------------------------------------------------------------------ */
tree *the_treep = NULL;
int worker_thread_num = 0;
const char *nvm_file_name = NULL;
bool debug_test = false;

#ifdef INSTRUMENT_INSERTION
int insert_total;         // insert_total=
int insert_no_split;      //              insert_no_split
int insert_leaf_split;    //             +insert_leaf_split
int insert_nonleaf_split; //            +insert_nonleaf_split
int total_node_splits;    // an insertion may cause multiple node splits
#endif                    // INSTRUMENT_INSERTION

#ifdef NVMFLUSH_STAT
NVMFLUSH_STAT_DEFS;
#endif
