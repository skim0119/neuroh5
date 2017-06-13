// -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//==============================================================================
///  @file append_tree.cc
///
///  Append tree structures to NeuroH5 file.
///
///  Copyright (C) 2016-2017 Project NeuroH5.
//==============================================================================

#include <mpi.h>
#include <hdf5.h>

#include <cassert>
#include <vector>

#include "neuroh5_types.hh"
#include "file_access.hh"
#include "rank_range.hh"
#include "dataset_num_elements.hh"
#include "enum_type.hh"
#include "path_names.hh"
#include "write_template.hh"
#include "cell_index.hh"

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
     )
    {
      herr_t status; hid_t wapl;

      unsigned int rank, size;
      assert(MPI_Comm_size(comm, (int*)&size) >= 0);
      assert(MPI_Comm_rank(comm, (int*)&rank) >= 0);

      /* Create property list for collective dataset write. */
      wapl = H5Pcreate (H5P_DATASET_XFER);
      status = H5Pset_dxpl_mpio (wapl, H5FD_MPIO_COLLECTIVE);

      /* Create HDF5 enumerated type for reading SWC type information */
      hid_t hdf5_swc_type = hdf5::create_H5Tenum<SWC_TYPE_T> (swc_type_enumeration);
    
      uint64_t all_attr_size=0, all_sec_size=0, all_topo_size=0;
      std::vector<uint64_t> attr_size_vector, sec_size_vector, topo_size_vector;

      std::vector<SEC_PTR_T> sec_ptr;
      std::vector<TOPO_PTR_T> topo_ptr;
      std::vector<ATTR_PTR_T> attr_ptr;
    
      std::vector<CELL_IDX_T> all_index_vector;
      std::vector<SECTION_IDX_T> all_src_vector, all_dst_vector;
      std::vector<COORD_T> all_xcoords, all_ycoords, all_zcoords;  // coordinates of nodes
      std::vector<REALVAL_T> all_radiuses;    // Radius
      std::vector<LAYER_IDX_T> all_layers;        // Layer
      std::vector<SECTION_IDX_T> all_sections;    // Section
      std::vector<PARENT_NODE_IDX_T> all_parents; // Parent
      std::vector<SWC_TYPE_T> all_swc_types; // SWC Types

      attr_ptr.push_back(0);
      sec_ptr.push_back(0);
      topo_ptr.push_back(0);

      hsize_t local_ptr_size = tree_list.size();
      if (rank == size-1)
        {
          local_ptr_size=local_ptr_size+1;
        }

      std::vector<uint64_t> ptr_size_vector;
      ptr_size_vector.resize(size);
      status = MPI_Allgather(&local_ptr_size, 1, MPI_UINT64_T, &ptr_size_vector[0], 1, MPI_UINT64_T, comm);
      assert(status == MPI_SUCCESS);
      hsize_t local_ptr_start = ptr_start;

      for (size_t i=0; i<rank; i++)
        {
          local_ptr_start = local_ptr_start + ptr_size_vector[i];
        }

      hsize_t global_ptr_size = ptr_start;

      for (size_t i=0; i<size; i++)
        {
          global_ptr_size = global_ptr_size + ptr_size_vector[i];
        }

      size_t block  = tree_list.size();

      for (size_t i = 0; i < block; i++)
        {
          neurotree_t &tree = tree_list[i];

          hsize_t attr_size=0, sec_size=0, topo_size=0;

          const CELL_IDX_T &gid = get<0>(tree);
          const std::vector<SECTION_IDX_T> & src_vector=get<1>(tree);
          const std::vector<SECTION_IDX_T> & dst_vector=get<2>(tree);
          const std::vector<SECTION_IDX_T> & sections=get<3>(tree);
          const std::vector<COORD_T> & xcoords=get<4>(tree);
          const std::vector<COORD_T> & ycoords=get<5>(tree);
          const std::vector<COORD_T> & zcoords=get<6>(tree);
          const std::vector<REALVAL_T> & radiuses=get<7>(tree);
          const std::vector<LAYER_IDX_T> & layers=get<8>(tree);
          const std::vector<PARENT_NODE_IDX_T> & parents=get<9>(tree);
          const std::vector<SWC_TYPE_T> & swc_types=get<10>(tree);

          topo_size = src_vector.size();
          assert(src_vector.size() == topo_size);
          assert(dst_vector.size() == topo_size);

          topo_ptr.push_back(topo_size+topo_ptr.back());
        
          attr_size = xcoords.size();
          assert(xcoords.size()  == attr_size);
          assert(ycoords.size()  == attr_size);
          assert(zcoords.size()  == attr_size);
          assert(radiuses.size() == attr_size);
          assert(layers.size()   == attr_size);
          assert(parents.size()  == attr_size);
          assert(swc_types.size()  == attr_size);

          attr_ptr.push_back(attr_size+attr_ptr.back());

          sec_size = sections.size();
          sec_ptr.push_back(sec_size+sec_ptr.back());

          all_index_vector.push_back(gid);
          all_src_vector.insert(all_src_vector.end(),src_vector.begin(),src_vector.end());
          all_dst_vector.insert(all_dst_vector.end(),dst_vector.begin(),dst_vector.end());
          all_sections.insert(all_sections.end(),sections.begin(),sections.end());
          all_xcoords.insert(all_xcoords.end(),xcoords.begin(),xcoords.end());
          all_ycoords.insert(all_ycoords.end(),ycoords.begin(),ycoords.end());
          all_zcoords.insert(all_zcoords.end(),zcoords.begin(),zcoords.end());
          all_radiuses.insert(all_radiuses.end(),radiuses.begin(),radiuses.end());
          all_layers.insert(all_layers.end(),layers.begin(),layers.end());
          all_parents.insert(all_parents.end(),parents.begin(),parents.end());
          all_swc_types.insert(all_swc_types.end(),swc_types.begin(),swc_types.end());

          all_attr_size = all_attr_size + attr_size;
          all_sec_size  = all_sec_size + sec_size;
          all_topo_size = all_topo_size + topo_size;

        }

      assert(all_index_vector.size() == block);
      assert(topo_ptr.size() == block+1);
      assert(sec_ptr.size()  == block+1);
      assert(attr_ptr.size() == block+1);
    
      attr_size_vector.resize(size);
      sec_size_vector.resize(size);
      topo_size_vector.resize(size);

      // establish the extents of data for all ranks
      status = MPI_Allgather(&all_attr_size, 1, MPI_UINT64_T, &attr_size_vector[0], 1, MPI_UINT64_T, comm);
      status = MPI_Allgather(&all_sec_size, 1, MPI_UINT64_T, &sec_size_vector[0], 1, MPI_UINT64_T, comm);
      status = MPI_Allgather(&all_topo_size, 1, MPI_UINT64_T, &topo_size_vector[0], 1, MPI_UINT64_T, comm);
      assert(status >= 0);

      hsize_t local_index_size = all_index_vector.size();

      std::vector<uint64_t> index_size_vector;
      index_size_vector.resize(size);
      status = MPI_Allgather(&local_index_size, 1, MPI_UINT64_T, &index_size_vector[0], 1, MPI_UINT64_T, comm);
      assert(status == MPI_SUCCESS);

      hsize_t local_index_start = ptr_start;
      for (size_t i=0; i<rank; i++)
        {
          local_index_start = local_index_start + index_size_vector[i];
        }
      
      hsize_t local_attr_start=attr_start, local_sec_start=sec_start, local_topo_start=topo_start;
      // calculate the starting position of this rank
      for (size_t i=0; i<rank; i++)
        {
          local_attr_start = local_attr_start + attr_size_vector[i];
          local_sec_start  = local_sec_start  + sec_size_vector[i];
          local_topo_start = local_topo_start + topo_size_vector[i];
        }
      // calculate the new sizes of the datasets
      hsize_t global_index_size=ptr_start, global_attr_size=attr_start, global_sec_size=sec_start, global_topo_size=topo_start;
      for (size_t i=0; i<size; i++)
        {
          global_index_size = global_index_size + index_size_vector[i];
          global_attr_size  = global_attr_size  + attr_size_vector[i];
          global_sec_size   = global_sec_size   + sec_size_vector[i];
          global_topo_size  = global_topo_size  + topo_size_vector[i];
        }
    

      // calculate the pointer positions relative to the local pointer starting position 
      for (size_t i=0; i<block+1; i++)
        {
          topo_ptr[i] = topo_ptr[i] + local_topo_start;
          sec_ptr[i]  = sec_ptr[i]  + local_sec_start;
          attr_ptr[i] = attr_ptr[i] + local_attr_start;
        }

      // create the cell index if option create_index is true
      if (create_index)
        {
          status = append_cell_index (comm, file_name, pop_name, all_index_vector, ptr_start);
        }
      else
        {
          // TODO: validate cell index
        }

      hid_t file = hdf5::open_file(comm, file_name, true);
      assert(file >= 0);


      
      status = hdf5::write<CELL_IDX_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::CELL_INDEX),
                                        global_index_size, local_index_start, local_index_size,
                                        CELL_IDX_H5_NATIVE_T, all_index_vector, wapl);


      status = hdf5::write<ATTR_PTR_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::ATTR_PTR),
                                        global_ptr_size, local_ptr_start, attr_ptr.size(),
                                        ATTR_PTR_H5_NATIVE_T,
                                        attr_ptr, wapl);
    
      status = hdf5::write<SEC_PTR_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::SEC_PTR),
                                       global_ptr_size, local_ptr_start, sec_ptr.size(),
                                       SEC_PTR_H5_NATIVE_T,
                                       sec_ptr, wapl);
      
      status = hdf5::write<TOPO_PTR_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::TOPO_PTR),
                                        global_ptr_size, local_ptr_start, topo_ptr.size(),
                                        TOPO_PTR_H5_NATIVE_T,
                                        topo_ptr, wapl);
      
      status = hdf5::write<SECTION_IDX_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::SRCSEC),
                                           global_topo_size, local_topo_start, all_src_vector.size(),
                                           SECTION_IDX_H5_NATIVE_T,
                                           all_src_vector, wapl);
      assert(status == 0);
    
      status = hdf5::write<SECTION_IDX_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::DSTSEC),
                                           global_topo_size, local_topo_start, all_dst_vector.size(),
                                           SECTION_IDX_H5_NATIVE_T,
                                           all_dst_vector, wapl);
      assert(status == 0);

      status = hdf5::write<SECTION_IDX_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::SECTION),
                                     global_sec_size, local_sec_start, all_sections.size(),
                                     SECTION_IDX_H5_NATIVE_T,
                                     all_sections, wapl);
      assert(status == 0);

      status = hdf5::write<COORD_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::X_COORD),
                                     global_attr_size, local_attr_start, all_xcoords.size(),
                                     COORD_H5_NATIVE_T,
                                     all_xcoords, wapl);
      assert(status == 0);
    
      status = hdf5::write<COORD_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::Y_COORD),
                                     global_attr_size, local_attr_start, all_ycoords.size(),
                                     COORD_H5_NATIVE_T,
                                     all_ycoords, wapl);
      assert(status == 0);
    
      status = hdf5::write<COORD_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::Z_COORD),
                                     global_attr_size, local_attr_start, all_zcoords.size(),
                                     COORD_H5_NATIVE_T,
                                     all_zcoords, wapl);
      assert(status == 0);
    
      status = hdf5::write<REALVAL_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::RADIUS),
                                       global_attr_size, local_attr_start, all_radiuses.size(),
                                       REAL_H5_NATIVE_T,
                                       all_radiuses, wapl);
      assert(status == 0);
    
      status = hdf5::write<LAYER_IDX_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::LAYER),
                                         global_attr_size, local_attr_start, all_layers.size(),
                                         LAYER_IDX_H5_NATIVE_T,
                                         all_layers, wapl);
      assert(status == 0);
    
      status = hdf5::write<PARENT_NODE_IDX_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::PARENT),
                                               global_attr_size, local_attr_start, all_layers.size(),
                                               PARENT_NODE_IDX_H5_NATIVE_T,
                                               all_parents, wapl);
      assert(status == 0);
    
      status = hdf5::write<SWC_TYPE_T> (file, hdf5::cell_attribute_path(hdf5::TREES, pop_name, hdf5::SWCTYPE),
                                        global_attr_size, local_attr_start, all_swc_types.size(),
                                        hdf5_swc_type,
                                        all_swc_types, wapl);
      assert(status == 0);
    
      status = hdf5::close_file(file);
      assert(status == 0);
      status = H5Pclose(wapl);
      assert(status == 0);
      status = H5Tclose(hdf5_swc_type);
      assert(status == 0);
    
      return 0;
    }
  }
}
