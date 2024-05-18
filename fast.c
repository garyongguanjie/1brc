// Note occassionally segfaults
// unfortunately abit hard to debug multithreaded programs with alot of shared memory

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <ctype.h>

#define hashSize 8192
#define NUMTHREADS 16
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAXCITIES 10000
#define lineSize 256

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

static struct City cityArray[MAXCITIES];
static int cityArrayP=0;
static struct City* cityArrayTable[MAXCITIES];
static pthread_mutex_t cityArrayPLock;

static struct Node nodeArr[MAXCITIES];
static int nodeP = 0;

// dont need mutex lock here because everytime we allocate new city we use the cityArrayPlock as well
static char charBuffer[MAXCITIES*100];
static int charP = 0;

static struct Node* hashTable[hashSize];
// array of mutex locks
static pthread_mutex_t hashMapLock[hashSize];
static pthread_mutex_t globalLock;

static pthread_t tid[NUMTHREADS];

struct pthreadargs{
    long l;
    long r;
    char* filecontents;
    int c;
};

struct Node* createNode(char *cityName,int cityNameLen,double temp){
    // printf("lock createnode \n");
    struct Node* p;
    p = &nodeArr[nodeP++];
    p->city = &cityArray[cityArrayP++];
    p->city->name = &charBuffer[charP];
    charP+=cityNameLen;
    strncpy(p->city->name,cityName,cityNameLen);
    p->city->min_t = temp; 
    p->city->max_t = temp;
    p->city->mean_t = 0;
    p->city->total_t = temp;
    p->city->count = 1;
    p->next = NULL;
    // printf("unlock createnode \n");
    return p;
}

int insertTemp(char *cityName,int cityNameLen,unsigned hashKey,double temp){
    // printf("lock hashmap\n");
    struct Node *p = hashTable[hashKey];
    struct Node *prev;
    if (p==NULL){
        pthread_mutex_lock(&cityArrayPLock);
        // printf("City Created: %s with length %d\n",cityName,cityNameLen);
        hashTable[hashKey] = createNode(cityName,cityNameLen,temp);
        pthread_mutex_unlock(&cityArrayPLock);
        // printf("unlock hashmap\n");
        return 1;
    }else{
        while(p!=NULL){
            if (strcmp(p->city->name,cityName)==0){
                p->city->min_t = MIN(temp,p->city->min_t); 
                p->city->max_t = MAX(temp,p->city->max_t);
                p->city->total_t += temp;
                p->city->count++;
                // printf("unlock hashmap\n");
                return 0;
            }
            prev = p;
            p = p->next;
        }
        prev->next = createNode(cityName,cityNameLen,temp);
        // printf("unlock hashmap\n");
        return 1;
    }
}


void* process(void *arg){
    struct pthreadargs *pta = (struct pthreadargs*)arg;
    long i;
    int sign,lineP,readCity,dot;
    char line[lineSize];
    double temp;
    unsigned hashKey;
    int count;
    count = 0;
    readCity = 1;
    lineP = 0;
    temp = 0;
    dot = 0;
    sign = 1;

    // printf("thread: l:%ld r:%ld\n",pta->l,pta->r);
    
    for(i=pta->l;i<pta->r;i++){
        if (pta->filecontents[i]==';'){
            line[lineP] = '\0';
            readCity = 0;
            continue;
        }
        if (pta->filecontents[i]=='\n'){
            // fine because final line ends with \n
            temp*=sign;
            hashKey = hash(line);
            // printf("%s %.1f\n",line,temp);
            pthread_mutex_lock(&hashMapLock[hashKey]);
            count+=insertTemp(line,lineP+1,hashKey,temp);
            pthread_mutex_unlock(&hashMapLock[hashKey]);
            readCity = 1;
            lineP = 0;
            temp = 0;
            dot = 0;
            sign = 1;
            continue;
        }
        if (readCity){
            line[lineP++] = pta->filecontents[i];
        }else if(pta->filecontents[i]=='.'){
            dot = 1;
        }else if (dot){
            temp += ((double)(pta->filecontents[i] - '0'))/10;
        }else{
            if (isdigit(pta->filecontents[i])){
                temp*=10;
                temp+= pta->filecontents[i] - '0';
            }else if (pta->filecontents[i]=='-'){
                // printf("reached\n");
                sign = -1;
            }else{
                printf("error");
            }

        }
    }
    // printf("DONE\n");
    
    pta->c = count;
    // printf("count in thread is %d\n",pta->c);
    return NULL;
}

int cityCmp(const void *a,const void *b){
    struct City *cityA = *(struct City **)a;
    struct City *cityB = *(struct City **)b;
    return strcmp(cityA->name,cityB->name);
}

int main(){
    char* filecontents;
    int fd,offsetP,cityCount,temp;
    long l,r,i,interval;
    struct pthreadargs pta[NUMTHREADS];
    struct Node* p;
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
    // init locks
    if (pthread_mutex_init(&cityArrayPLock, NULL) != 0) { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    } 
    if (pthread_mutex_init(&globalLock, NULL) != 0) { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    } 
    for(i=0;i<hashSize;i++){
        if (pthread_mutex_init(&hashMapLock[i], NULL) != 0) { 
            printf("\n mutex init has failed\n"); 
            return 1; 
        } 
    }

    filecontents = (char*)mmap(NULL,sb.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    close(fd);
    // printf("Size of file is %ld\n",sb.st_size);

    
    interval = sb.st_size/NUMTHREADS;
    // printf("Interval is %ld\n",interval);
    l=0;
    offsetP = 0;
    for(i=0;i<NUMTHREADS-1;i++){
        // printf("i: %ld interval %ld\n",i,interval);
        r = (i+1)*interval;
        
        while(filecontents[r]!='\n'){
            r++;
        };
        // printf("l is %ld r is %ld\n",l,r);
        pta[offsetP].l = l;
        pta[offsetP].r = r;
        pta[offsetP].filecontents = filecontents;
        offsetP++;
        l = r+1;
    }

    r = sb.st_size;
    pta[offsetP].l = l;
    pta[offsetP].r = r;
    pta[offsetP].filecontents = filecontents;

    for(i=0;i<NUMTHREADS;i++){
        pthread_create(&tid[i],NULL,process,&pta[i]);
    }

    for(i=0;i<NUMTHREADS;i++){
        pthread_join(tid[i],NULL);
    }
    cityCount = 0;
    for(i=0;i<NUMTHREADS;i++){
        // printf("count outside is: %d\n",pta[i].c);
        cityCount += pta[i].c;
    }
    // printf("City Count is %d\n",cityCount);

    for(i=0;i<hashSize;i++){
        p = hashTable[i];
        while (p!=NULL){
            p->city->mean_t = p->city->total_t/p->city->count;
            p = p->next;
        }
    }

    for(i=0;i<cityCount;i++){
        cityArrayTable[i] = &cityArray[i];
    }
    // printf("sorting...\n");
    qsort(cityArrayTable,cityCount,sizeof(struct City*),cityCmp);
    
    printf("{");
    for(i=0;i<cityCount;i++){
        printf("%s=%.1f/%.1f/%.1f",cityArrayTable[i]->name,cityArrayTable[i]->min_t,cityArrayTable[i]->mean_t,cityArrayTable[i]->max_t);
        if (i<cityCount-1){
            printf(", ");
        }
    }
    printf("}\n");
    return 0;
}