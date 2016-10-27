// -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//==============================================================================
///  @file dbs_edge_reader.cc
///
///  Functions for reading edge information in DBS (Destination Block Sparse) format.
///
///  Copyright (C) 2016 Project Neurograph.
//==============================================================================

#ifndef DBS_GRAPH_READER_HH
#define DBS_GRAPH_READER_HH

#include "ngh5types.hh"

#include "mpi.h"

#include <map>
#include <vector>

namespace ngh5
{

extern herr_t read_projection_names
(
 MPI_Comm                 comm,
 const std::string&       file_name, 
 std::vector<std::string> &prj_vector
 );

extern herr_t read_dbs_projection
(
 MPI_Comm                 comm,
 const std::string&       file_name, 
 const std::string&       proj_name, 
 const std::vector<pop_range_t> &pop_vector,
 NODE_IDX_T&         dst_start,
 NODE_IDX_T&         src_start,
 uint64_t            &nedges, /* total number of edges in the projection */
 DST_BLK_PTR_T&      block_base, /* global index of the first block read by this task */
 DST_PTR_T&          edge_base,  /* global index of the first edge read by this task */
 std::vector<DST_BLK_PTR_T>&  dst_blk_ptr,  
 std::vector<NODE_IDX_T>& dst_idx,
 std::vector<DST_PTR_T>&  dst_ptr,  /* one longer than owned nodes count */
 std::vector<NODE_IDX_T>& src_idx
 );
  
}

#endif
