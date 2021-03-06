#include <stdio.h>
#include <string.h>
#include <openssl/md5.h>
#include <malloc.h>
#include <mpi.h>

#define NUM_CHARACTER 36 // a-z, 0-9 ==> 36 character.
#define NLOOPS 4000  // divide each 4000 password for worker, then check finded pass.

/*  Convert a number to string  (generate password by number)   */
char *number_to_password(unsigned int num) {

    /* Character sort by frequency appears in Vietnamese alphabet */
    char character[NUM_CHARACTER] =
        { '0', 'n', 'a', 'o', 'h', 'i', 'u', 'g', 't', 'c'
        , 'e', 'd', 'm', 'y', 'l', 'r', 'b', 'v', 's', 'k'
        , 'p', 'x', 'q', '1', '2', '3', '4', '5', '6', '7'
        , '8', '9', 'f', 'j', 'w', 'z'
        } ; 
    
    char *password = (char *)malloc(sizeof(char) * 20);
    int index_char[20] ;

    int i = 0 ;
    unsigned int temp ;
    while(num !=0) {
        temp = num%NUM_CHARACTER ;
        index_char[i] = temp;
        i++ ;
        num = num/NUM_CHARACTER ;
    }

    int j, k=0 ;
    for(j=i-1 ; j>=0; j--, k++ )
        password[k] = character[ index_char[j]] ;

    password[i] = '\0' ;

    return password;
}

/* Convert password string to MD5 string */
char *password_to_md5(char *password) {
    char *md5 = (char *)malloc(33);
    unsigned char digest[16];
    int length = strlen(password);

    MD5((unsigned char *)password, length, (unsigned char *)&digest);

    int i;
    for (i = 0; i < 16; ++i)
        sprintf(&md5[i * 2], "%02x", (unsigned int)digest[i]);

    return md5;
}

int finding_pass (int number, char *md5_input) {

    int is_find = 0 ; 
    char *password = number_to_password(number);
    char *md5_password = password_to_md5(password);

    if (strcmp(md5_input, md5_password) == 0) {
        printf("YOUR PASSWORD: %s\n", password);
        is_find = 1; 
    }
    free(password);
    free(md5_password);
    return is_find ;
}

int main() {

    char *md5_input = (char *) malloc (sizeof(char)*33) ;
    int rank, ntasks;
    unsigned int i, j ;

    MPI_Status status ;
    int is_find = 0 ; 
    int sum_is_find = 0 ; // using in MPI_Reduce.

    MPI_Init(NULL, NULL) ;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank) ;
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks) ;

    unsigned int start, end; // divide work to worker
    int count, remain ; 
    double start_time, end_time ; // count time

    unsigned int current = 0 ; // number password decoded in current.

    if(rank== 0) { // Master scan MD5 hash

        printf("\n ------- MD5 DECODE BY LINHPHAN -------\n\n") ;
        printf("ENTER YOUR MD5 HASH: ") ;
        scanf("%s", md5_input) ;
    }

    // Master broadcast md5 hash string to all worker
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(md5_input, 33, MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

     start_time = MPI_Wtime() ;

    while(!is_find) { // stop find password when is_find = 1

        count = NLOOPS/ ntasks ; 
        start = current + rank*count ;
        end = start + count ;

        for(i = start; i<end; i++)
            is_find += finding_pass(i, md5_input) ;

        if(rank==0) { // master caculate remaining password.
            remain = current + count*ntasks ;
            for(i=remain; i<current + NLOOPS; i++)
                is_find += finding_pass(i, md5_input) ;
        }
        current += NLOOPS ;
        
        // Reduce all is_find into sum_is_find, and send it to all worker.
        MPI_Reduce(&is_find, &sum_is_find, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD ) ;
        is_find = sum_is_find ;

        MPI_Barrier(MPI_COMM_WORLD) ;
        MPI_Bcast(&is_find, 1, MPI_INT, 0, MPI_COMM_WORLD) ;
        MPI_Barrier(MPI_COMM_WORLD) ;

    }
    free(md5_input) ;
    end_time = MPI_Wtime() ;
    if(rank==0) {
        printf("Total time: %f seconds !\n", end_time -start_time) ;
        printf("\n--------END PROGRAME ---------\n");
    }

    MPI_Finalize() ;

    return 0;
}

/*
       FOR TEST CASE: 
       MD5: 900150983cd24fb0d6963f7d28e17f72  ==> pass: abc
       MD5: 892da3d819056410c05bca7747d22735  ==> pass: linh
       MD5: b2157e7b2ae716a747597717f1efb7a0  ==> pass: abc12
       MD5: c566c0fed4c02cf3751b4448df9be909  ==> pass: nha01
       MD5: 39aed6dcb42493d547e209d195a4dddb  ==> pass: nhacsy
       MD5: 02c2a68bffd54e229609ccc808cb45d7  ==> pass: it4865


    */