// -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//==============================================================================
///  @file graph_reader.cc
///
///  Top-level functions for reading graphs in DBS (Destination Block Sparse) format.
///
///  Copyright (C) 2016 Project Neurograph.
//==============================================================================


#ifndef GRAPH_SCATTER_HH
#define GRAPH_SCATTER_HH

#include "ngh5types.hh"

#include <mpi.h>
#include <hdf5.h>

#include <map>
#include <vector>

namespace ngh5
{

int read_graph
(
 MPI_Comm comm,
 const std::string& file_name,
 const bool opt_attrs,
 const std::vector<std::string> prj_names,
 std::vector<prj_tuple_t> &prj_list,
 size_t &local_prj_num_edges,
 size_t &total_prj_num_edges
 );

int scatter_graph
(
 MPI_Comm all_comm,
 const std::string& file_name,
 const int io_size,
 const bool opt_attrs,
 const std::vector<std::string> prj_names,
  // A vector that maps nodes to compute ranks
 const std::vector<rank_t> node_rank_vector,
 std::vector < edge_map_t > & prj_vector
 );

  
}

#endif
