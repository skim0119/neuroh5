
#include "ngh5paths.h"
#include "ngh5types.h"

#include "crs_graph_reader.hh"

#include "hdf5.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <vector>
#include <mpi.h>

using namespace std;

/*****************************************************************************
 * Create a list of edges (represented as src/dst node pairs)
 *****************************************************************************/

int create_edge_list
(
  const NODE_IDX_T&         base,
  const vector<ROW_PTR_T>&  row_ptr,
  const vector<NODE_IDX_T>& col_idx,
  vector<NODE_IDX_T>&       edge_list
)
{
  int ierr = 0;

  for (size_t i = 0; i < row_ptr.size(); ++i)
  {
    NODE_IDX_T row = base + (NODE_IDX_T)i;
    if (i < row_ptr.size()-1)
    {
      size_t low = row_ptr[i], high = row_ptr[i+1];
      for (size_t j = low; j < high; ++j)
      {
        NODE_IDX_T col = col_idx[j];
        edge_list.push_back(row);
        edge_list.push_back(col);
      }
    }
  }

  return ierr;
}


/*****************************************************************************
 * Main driver
 *****************************************************************************/

#define FNAME "crs.h5"

int main(int argc, char** argv)
{
  assert(MPI_Init(&argc, &argv) >= 0);

  int rank, size;
  assert(MPI_Comm_size(MPI_COMM_WORLD, &size) >= 0);
  assert(MPI_Comm_rank(MPI_COMM_WORLD, &rank) >= 0);

  // parse arguments

  // read the file

  NODE_IDX_T base;
  vector<ROW_PTR_T> row_ptr;
  vector<NODE_IDX_T> col_idx;
  assert(read_crs_graph(MPI_COMM_WORLD, FNAME, base, row_ptr, col_idx) >= 0);

  // create the partitioner input

  vector<NODE_IDX_T> edge_list;
  assert(create_edge_list(base, row_ptr, col_idx, edge_list) >= 0);
  assert(edge_list.size()%2 == 0);

  // partition the graph

  
  MPI_Finalize();
  return 0;
}
