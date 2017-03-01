#!/bin/bash
#
#SBATCH -J neurograph_scatter_attrs_test
#SBATCH -o ./results/neurograph_scatter_attrs_test.%j.o
#SBATCH --nodes=64
#SBATCH --ntasks-per-node=16
#SBATCH -p compute
#SBATCH -t 1:00:00
#SBATCH --mail-user=ivan.g.raikov@gmail.com
#SBATCH --mail-type=END
#

module load python
module load hdf5
module load scipy
module load mpi4py

export PYTHONPATH=/opt/python/lib/python2.7/site-packages:$PYTHONPATH
export PYTHONPATH=$HOME/.local/lib/python2.7/site-packages:$PYTHONPATH

set -x

nodefile=`generate_pbs_nodefile`

mpirun_rsh -export-all -hostfile $nodefile -np 1024  \
PATH=$PATH LD_LIBRARY_PATH=$LD_LIBRARY_PATH PYTHONPATH=$PYTHONPATH \
/opt/python/bin/python ./tests/comet_scatter_test.py

echo All done!


