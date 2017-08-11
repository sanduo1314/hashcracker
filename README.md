# hashcracker
OpenMPI based distributed password cracker

## 1.Make sure you have the right packages in place 
$ sudo apt-get install build-essential libopenmpi-dev openmpi-bin libssl-dev

## 2.Compile with 
$ mpicc -O3 crackzor.c -o crackzor -lm -lssl -lcrypto

## 3.Create a file called “machines” containing a newline separated list of every machine that are in your cluster,for exmaple:
computer-01
computer-02
computer-03
computer-04

## 4.Open MPI uses SSH for communication between nodes, as such, you need to make sure that the node you will be launching crakzor from is able to do SSH key based authentication to all the other nodes in the cluster. For example:

$ ssh computer-01

## 5.You now need to disseminate your executable across all the machines that will be running it.if you have a cluster,please post you r program in jobs commond. 
$ for machine in `cat machines`; do scp crackzor $machine:~; done

## 6.Run with: 
$ mpirun -npernode Y -machinefile machines crackzor fbade9e36a3f36d3d676c1b808451dd7 abcdefghijklmnopqrstuvwxzy 1 1
where Y is the number of cores each machine in your cluster has. If you are running this on machines with 2 CPUs with 8 cores each, Y = 8 * 2 = 16.
