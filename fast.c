#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define hashSize 1024
#define NUMTHREADS 12
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

struct City {
    double min_t;
    double max_t;
    double mean_t;
    double total_t;
    int count;
    char *name;
};

struct Node{
    struct City *city;
    struct Node *next;
};

unsigned hash(char *s)
{
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++)
        hashval = *s + 31*hashval;
    return hashval % hashSize;
}

int main(){
    char* filecontents;
    int fd,offsetP;
    long l,r,i,interval;

    int offsetSplit[NUMTHREADS][2];
    
    struct stat sb;
    fd = open("measurements.txt",O_RDONLY);
    if (fd==-1){
        printf("Cannot read file\n");
        return 1;
    }
    if(fstat(fd,&sb)==-1){
        printf("Cannot fstat\n");
        return -1;
    }
    filecontents = (char*)mmap(NULL,sb.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    close(fd);
    printf("Size of file is %ld\n",sb.st_size);

    
    interval = sb.st_size/NUMTHREADS;
    // printf("Interval is %ld\n",interval);
    l=0;
    offsetP = 0;
    for(i=0;i<NUMTHREADS-1;i++){
        // printf("i: %ld interval %ld\n",i,interval);
        r = i*interval;
        // printf("r is %ld\n",r);
        while(filecontents[r]!='\n'){
            r++;
        };
        offsetSplit[offsetP][0] = l;
        offsetSplit[offsetP][1] = r;
        offsetP++;
        l = r;
    }
    printf("offsetP %d\n",offsetP);
    r = sb.st_size;
    offsetSplit[offsetP][0] = l;
    offsetSplit[offsetP][1] = r;

    for(i=0;i<NUMTHREADS;i++){
        l = offsetSplit[i][0];
        r = offsetSplit[offsetP][1];
        // split pthreads
    }
    return 0;
}