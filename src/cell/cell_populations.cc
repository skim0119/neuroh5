// -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//==============================================================================
///  @file cell_populations.cc
///
///  Functions for reading population names from an HDF5 enumerated type.
///
///  Copyright (C) 2016-2017 Project Neurotrees.
//==============================================================================

#include "debug.hh"

#include <hdf5.h>

#include <cstring>
#include <vector>
#include <set>

#include "neuroh5_types.hh"
#include "bcast_string_vector.hh"
#include "path_names.hh"

#undef NDEBUG
#include <cassert>

#define MAX_POP_NAME_LEN 1024

using namespace std;

namespace neuroh5
{

  namespace cell
  {
  
    
    //////////////////////////////////////////////////////////////////////////
    herr_t iterate_cb
    (
     hid_t             grp,
     const char*       name,
     const H5L_info_t* info,
     void*             op_data
     )
    {
      vector<string>* ptr = (vector<string>*)op_data;
      ptr->push_back(string(name));
      return 0;
    }
    
    //////////////////////////////////////////////////////////////////////////
    herr_t read_population_names
    (
     MPI_Comm             comm,
     const std::string&   file_name,
     vector<string>&      pop_names
     )
    {
      herr_t ierr = 0;
    
      int rank, size;
    
      assert(MPI_Comm_size(comm, &size) >= 0);
      assert(MPI_Comm_rank(comm, &rank) >= 0);
    
      // MPI rank 0 reads and broadcasts the names of populations
      hid_t grp = -1;

      // Rank 0 reads the names of populations and broadcasts
      if (rank == 0)
        {
          
          hid_t file = H5Fopen(file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
          assert(file >= 0);
          
          hsize_t num_populations;
          grp = H5Gopen(file, hdf5::POPULATIONS.c_str(), H5P_DEFAULT);
          assert(grp >= 0);
          assert(H5Gget_num_objs(grp, &num_populations)>=0);

          hsize_t idx = 0;
          vector<string> op_data;
          assert(H5Literate(grp, H5_INDEX_NAME, H5_ITER_NATIVE, &idx,
                            &iterate_cb, (void*)&op_data ) >= 0);
        
          assert(op_data.size() == num_populations);
        
          for (size_t i = 0; i < op_data.size(); ++i)
            {
              pop_names.push_back(op_data[i]);
            }
        
          assert(H5Gclose(grp) >= 0);
          assert(H5Fclose(file) >= 0);
        }

      ierr = mpi::bcast_string_vector (comm, 0,
                                       MAX_POP_NAME_LEN,
                                       pop_names);
      return ierr;
    }

    
    /*************************************************************************
     * Read the valid population combinations
     *************************************************************************/

    herr_t read_population_combos
    (
     MPI_Comm                   comm,
     const std::string&         file_name,
     set< pair<pop_t,pop_t> >&  pop_pairs
     )
    {
      herr_t ierr = 0;

      int rank, size;

      assert(MPI_Comm_size(comm, &size) >= 0);
      assert(MPI_Comm_rank(comm, &rank) >= 0);

      // MPI rank 0 reads and broadcasts the number of pairs

      uint64_t num_pairs;

      hid_t file = -1, dset = -1;

      // process 0 reads the number of pairs and broadcasts
      if (rank == 0)
        {
          file = H5Fopen(file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
          assert(file >= 0);

          dset = H5Dopen2(file, hdf5::h5types_path_join(hdf5::POP_COMBS).c_str(),
                          H5P_DEFAULT);
          assert(dset >= 0);
          hid_t fspace = H5Dget_space(dset);
          assert(fspace >= 0);
          num_pairs = (uint64_t) H5Sget_simple_extent_npoints(fspace);
          assert(num_pairs > 0);
          assert(H5Sclose(fspace) >= 0);
        }

      assert(MPI_Bcast(&num_pairs, 1, MPI_UINT64_T, 0, comm) == MPI_SUCCESS);

      // allocate buffers
      vector<pop_t> v(2*num_pairs);

      // MPI rank 0 reads and broadcasts the population pairs

      if (rank == 0)
        {
          vector<pop_comb_t> vpp(num_pairs);
          hid_t ftype = H5Dget_type(dset);
          assert(ftype >= 0);
          hid_t mtype = H5Tget_native_type(ftype, H5T_DIR_ASCEND);

          assert(H5Dread(dset, mtype, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                         &vpp[0]) >= 0);
          for (size_t i = 0; i < vpp.size(); ++i)
            {
              v[2*i]   = vpp[i].src;
              v[2*i+1] = vpp[i].dst;
            }

          assert(H5Tclose(mtype) >= 0);
          assert(H5Tclose(ftype) >= 0);

          assert(H5Dclose(dset) >= 0);
          assert(H5Fclose(file) >= 0);
        }

      assert(MPI_Bcast(&v[0], (int)2*num_pairs, MPI_UINT16_T, 0, comm) == MPI_SUCCESS);

      // populate the set
      pop_pairs.clear();
      for (size_t i = 0; i < v.size(); i += 2)
        {
          pop_pairs.insert(make_pair(v[i], v[i+1]));
        }

      return ierr;
    }

    /*************************************************************************
     * Read the population ranges
     *************************************************************************/

    herr_t read_population_ranges
    (
     MPI_Comm                                comm,
     const std::string&                      file_name,
     map<NODE_IDX_T, pair<uint32_t,pop_t> >& pop_ranges,
     vector<pop_range_t> &pop_vector,
     size_t &n_nodes
     )
    {
      herr_t ierr = 0;

      int rank, size;
      assert(MPI_Comm_size(comm, &size) >= 0);
      assert(MPI_Comm_rank(comm, &rank) >= 0);

      // MPI rank 0 reads and broadcasts the number of ranges

      uint64_t num_ranges;

      hid_t file = -1, dset = -1;

      // process 0 reads the number of ranges and broadcasts
      if (rank == 0)
        {
          file = H5Fopen(file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
          assert(file >= 0);
          dset = H5Dopen2(file, hdf5::h5types_path_join(hdf5::POPULATIONS).c_str(), H5P_DEFAULT);
          assert(dset >= 0);

          hid_t fspace = H5Dget_space(dset);
          assert(fspace >= 0);
          num_ranges = (uint64_t) H5Sget_simple_extent_npoints(fspace);
          assert(num_ranges > 0);
          assert(H5Sclose(fspace) >= 0);

          hid_t ftype = H5Dget_type(dset);
          assert(ftype >= 0);

          pop_vector.resize(num_ranges);
          hid_t mtype = H5Tget_native_type(ftype, H5T_DIR_ASCEND);
          assert(H5Dread(dset, mtype, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                         &pop_vector[0]) >= 0);

          assert(H5Tclose(mtype) >= 0);
          assert(H5Tclose(ftype) >= 0);

          assert(H5Dclose(dset) >= 0);
          assert(H5Fclose(file) >= 0);
        }

      assert(MPI_Bcast(&num_ranges, 1, MPI_UINT64_T, 0, comm) >= 0);
      assert(num_ranges > 0);

      // allocate buffers
      pop_vector.resize(num_ranges);

      // MPI rank 0 reads and broadcasts the population ranges
      assert(MPI_Bcast(&pop_vector[0], (int)num_ranges*sizeof(pop_range_t),
                       MPI_BYTE, 0, comm) >= 0);

      n_nodes = 0;
      for(size_t i = 0; i < pop_vector.size(); ++i)
        {
          pop_ranges.insert(make_pair(pop_vector[i].start,
                                      make_pair(pop_vector[i].count,
                                                pop_vector[i].pop)));
          n_nodes = n_nodes + pop_vector[i].count;
        }

      return ierr;
    }


    /*************************************************************************
     * Read the population labels
     *************************************************************************/

    herr_t read_population_labels
    (
     MPI_Comm comm,
     const string& file_name,
     vector< pair<pop_t, string> > & pop_labels
     )
    {
      herr_t ierr = 0;

      int rank, size;
      assert(MPI_Comm_size(comm, &size) >= 0);
      assert(MPI_Comm_rank(comm, &rank) >= 0);

      // MPI rank 0 reads and broadcasts the number of ranges

      vector <string> pop_name_vector;

      // process 0 reads the number of populations and broadcasts
      if (rank == 0)
        {
          hid_t file = -1, pop_labels_type = -1, grp_h5types = -1;
            
          file = H5Fopen(file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
          assert(file >= 0);

          grp_h5types = H5Gopen2(file, hdf5::H5_TYPES.c_str(), H5P_DEFAULT);
          assert(grp_h5types >= 0);

          pop_labels_type = H5Topen(grp_h5types, hdf5::POP_LABELS.c_str(), H5P_DEFAULT);
          assert(pop_labels_type >= 0);

          size_t num_labels = H5Tget_nmembers(pop_labels_type);
          assert(num_labels > 0);

          for (size_t i=0; i<num_labels; i++)
            {
              char namebuf[MAX_POP_NAME_LEN];
              ierr = H5Tenum_nameof(pop_labels_type, &i, namebuf, MAX_POP_NAME_LEN);
              pop_name_vector.push_back(string(namebuf));
              assert(ierr >= 0);
            }
            
          assert(H5Tclose(pop_labels_type) >= 0);
          assert(H5Gclose(grp_h5types) >= 0);
          assert(H5Fclose(file) >= 0);
            
        }


      ierr = mpi::bcast_string_vector(comm, 0, MAX_POP_NAME_LEN, pop_name_vector);

      for (uint16_t i=0; i<pop_name_vector.size(); i++)
        {
          pop_labels.push_back(make_pair(i, pop_name_vector[i]));
        }
        
      return ierr;
    }



  }
}