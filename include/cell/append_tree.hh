#ifndef WRITE_TREE_HH
#define WRITE_TREE_HH

#include "hdf5.h"

#include <vector>

namespace neuroh5
{
  namespace cell
  {
    
    /*****************************************************************************
     * Save tree data structures to HDF5
     *****************************************************************************/
    int append_trees
    (
     MPI_Comm comm,
     const std::string& file_name,
     const std::string& pop_name,
     const hsize_t ptr_start,
     const hsize_t attr_start,
     const hsize_t sec_start,
     const hsize_t topo_start,
     std::vector<neurotree_t> &tree_list,
     bool create_index = false
     );
  }
}

#endif
