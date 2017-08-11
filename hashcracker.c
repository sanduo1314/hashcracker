#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <openssl/md5.h>


// function predefinition
void base_convert_from_decimal( long int number, int base, int depth, int* result ) ;


int main(int argc, char *argv[]) {

	// retrieving parameters
	if( argc!=5 ) {
		printf( "usage: %s <hash> <possible_characters> <min_depth> <max_depth>\n\n", argv[0] ) ;
		printf( "    - hash: a 32 character long MD5 hash, what you want to crack\n" ) ;
		printf( "    - possible_characters: a string of all the characters that will be used to compute all the permutations to try\n" ) ;
		printf( "    - min_depth: the minimum string size of a permutation\n\n" ) ;
		printf( "    - max_depth: the maximum string size of a permutation\n\n" ) ;
		printf( "EXAMPLE: %s 098f6bcd4621d373cade4e832627b4f6 ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01234567890 1 5\n\n", argv[0] ) ;
		printf( "EXAMPLE WITH MPI: mpirun -npernode 16 -machinefile machines %s 098f6bcd4621d373cade4e832627b4f6 ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01234567890 1 8\n", argv[0] ) ;
		printf( "   -machinefile is a newline separated file of all the machines in your cluster\n" ) ; 
		printf( "   -npernode tells OpenMPI how many processes to run on each machine, an optimal value there is the number of cores/CPUs you have in each machine. For example if a cluster is made of machines with 2 CPUs with 8 cores each, 16 is the value you want there.\n\n" ) ; 
		exit( 1 ) ;
	}
	
	char* hash = argv[1] ;
	if( strlen(hash)!=32 ) {
		printf( "invalid hash size, hash needs to be 32 character long\n" ) ;
		exit( 1 ) ;
	}
	//printf( "%d\n",(int)strlen( hash) ) ;
	
	char* char_array = argv[2] ;
	// TODO check for repetition
	
	int min_depth = atoi( argv[3] ) ;
	if( min_depth<1 || min_depth>32 ) {
		printf( "invalid min_depth, must belong to [1,32]\n" ) ;
		exit( 1 ) ;
	}
	
	int max_depth = atoi( argv[4] ) ;
	if( max_depth<1 || max_depth>32 ) {
		printf( "invalid max_depth, must belong to [1,32]\n" ) ;
		exit( 1 ) ;
	}
	if( max_depth<min_depth ) {
		printf( "invalid max_depth, can't be smaller than min_depth\n" ) ;
		exit( 1 ) ;
	}
	
	int numprocs, rank, namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	
	// lame ass hex 2 dec routine
	unsigned char hash_hexed[16] ;
	int j ;
	int k = 0 ;
	for( j=0 ; j<32 ; j+=2 ) {
		unsigned char digit1 = hash[j] ;
		unsigned char digit2 = hash[j+1] ;
		if( digit1>=97 ) {
			digit1 -= 87 ;
		} else {
			digit1 -= 48 ;
		}
		if( digit2>=97 ) {
			digit2 -= 87 ;
		} else {
			digit2 -= 48 ;
		}

		hash_hexed[k] = digit1 * 16 + digit2 ;
		//printf( "%x", hash_hexed[k] ) ;
		k++ ;
	}

	// initializing MPI stuff
	MPI_Init( &argc, &argv ) ;
	MPI_Comm_size( MPI_COMM_WORLD, &numprocs ) ;
	MPI_Comm_rank( MPI_COMM_WORLD, &rank ) ;
	MPI_Get_processor_name( processor_name, &namelen ) ;

	// initializing md5 stuff
	MD5_CTX context;
	unsigned char md[MD5_DIGEST_LENGTH];

	//printf( "> process #%d on %s out of %d\n", rank, processor_name, numprocs ) ;
	
	int depth = min_depth ;
	while( depth<=max_depth ) {
		long int space = pow( (long int)strlen(char_array), (long int)depth ) ;
		//printf( "> sample space size: %ld\n", space ) ;
		//printf( "> numprocs: %ld\n", (long int)numprocs ) ;
		long int slice_size = (long int)ceil( (long double)space/(long double)numprocs ) ;
		//printf( "> #%d: depth %d / sample space size %ld / slice size %ld\n", rank, depth, space, slice_size ) ;
		long int progress = 0 ;
		//printf( "> slice_size: %Ld\n", slice_size ) ;
	
		if( slice_size>0 && (long double)(space/slice_size)>(long double)rank ) {
			int counters[depth] ;
			base_convert_from_decimal( (long int)rank*slice_size, strlen(char_array), depth, counters ) ;
			unsigned char sstring[depth+1] ; // you want an extra spot for the \0 char
			int k ;
			for( k=0 ; k<depth ; k++ ) {
				sstring[k] = char_array[counters[k]] ;
			}
			sstring[depth] = '\0' ;
			//printf( "> #%d: start %s\n", rank, sstring ) ;
			int end_counters[depth] ;
			base_convert_from_decimal( fmin(((long double)rank+1)*slice_size-1, space-1), strlen(char_array), depth, end_counters ) ;
			for( k=0 ; k<depth ; k++ ) {
				sstring[k] = char_array[end_counters[k]] ;
			}
			sstring[depth] = '\0' ;
			//printf( "> #%d: end %s\n", rank, sstring ) ;
	
			unsigned char string[depth+1] ; // you want an extra spot for the \0 char
			while( 1 ) {
				// display feedback every once in a while
				if( progress%100000000==0 ) {
					printf( "> %s / #%d / depth %d / %Lf%%\n", processor_name, rank, depth,(long double)progress/(long double)slice_size*100 ) ;
				}
				progress++ ;
	
				int i ;
				for( i=0 ; i<depth ; i++ ) {
					string[i] = char_array[counters[i]] ;
				}
				string[depth] = '\0' ;
				//printf( "%s\n", string ) ;
	
				MD5_Init(&context);
				MD5_Update(&context, string, depth);
				MD5_Final(md, &context);
				int match = 1 ;
				for (i = 0; i < 16; i++) {
					if( md[i]!=hash_hexed[i] ) {
						match = 0 ;
						break ;
					}
					//printf("%d ", md[i]);
				}
				if( match==1 ) {
					printf( "GOT IT: %s\n\n", string ) ;
					MPI_Abort( MPI_COMM_WORLD, 1 ) ;
				}
				//printf("\n");
	
				// have we reached the end?
				//printf( "have we reached the end?\n" ) ;
				match = 1 ;
				i = depth - 1 ;
				while( i>=0 ) {
					if( counters[i]!=end_counters[i] ) {
						match = 0 ;
						break ;
					}
					i-- ;
				}
				if( match==1 ) {
					break ;
				} else {
					// updating counters
					//printf( "updating counters\n" ) ;
					i = depth - 1 ;
					while( i>=0 ) {
						counters[i]++ ;
						if( counters[i]<strlen(char_array) ) {
							string[i] = char_array[counters[i]] ;
							break ;
						} else {
							counters[i] = 0 ;
							string[i] = char_array[counters[i]] ;
							i-- ;
						}
					}
				}
			}
		}
		
		depth++ ;
	}

	MPI_Finalize();
}


void base_convert_from_decimal( long int number, int base, int depth, int* result ) {
	int index = 0 ;
	while( number!=0 ) {
		result[index] = number % base ;
		//printf( "%d\n", number%to_base ) ;
		number /= base ;
		index++ ;
	}
	while( index<depth ) {
		result[index] = 0 ;
		index++ ;
	}
	
	// reversing
	index = 0 ;
	char temp ;
	while( index<(depth/2) ) {
		temp = result[index] ;
		result[index] = result[depth-index-1] ;
		result[depth-index-1] = temp ;
		index++ ;
	}
}
