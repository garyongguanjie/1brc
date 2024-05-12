#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define hashSize 1000
#define lineSize 100
#define bufferSize 50

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

struct Node* createNode(char *cityName,int cityNameLen,double temp){
    struct Node* p;
    p = malloc(sizeof(struct Node));
    p->city = malloc(sizeof(struct City));
    p->city->name = malloc(sizeof(char)*cityNameLen);
    strncpy(p->city->name,cityName,cityNameLen);
    p->city->min_t = temp; 
    p->city->max_t = temp;
    p->city->mean_t = 0;
    p->city->total_t = temp;
    p->city->count = 1;
    p->next = NULL;
    return p;
}

int insertTemp(struct Node **hashTable,char *cityName,int cityNameLen,unsigned hashKey,double temp){
    struct Node *p = hashTable[hashKey];
    struct Node *prev;
    if (p==NULL){
        hashTable[hashKey] = createNode(cityName,cityNameLen,temp);
        return 1;
    }else{
        while(p!=NULL){
            if (strcmp(p->city->name,cityName)==0){
                p->city->min_t = MIN(temp,p->city->min_t); 
                p->city->max_t = MAX(temp,p->city->max_t);
                p->city->total_t += temp;
                p->city->count++;
                return 0;
            }
            prev = p;
            p = p->next;
        }
        prev->next = createNode(cityName,cityNameLen,temp);
        return 1;
    }
}

int cityCmp(const void *a,const void *b){
    struct City *cityA = *(struct City **)a;
    struct City *cityB = *(struct City **)b;
    return strcmp(cityA->name,cityB->name);
}


int main(){
    int cityCount = 0;
    struct Node *hashTable[hashSize] = {NULL};
    char line[lineSize];
    char buffer[bufferSize];
    int semicolonI;
    int i;
    unsigned hashKey;
    double temp;
    struct City **cityArray;
    struct City **cap;
    struct Node *p;

    FILE *fp = fopen("measurements.txt","r");

    if (fp==NULL){
        printf("Cannot read file\n");
        return 1;
    }
    while (fgets(line,lineSize,fp)){
        for (i=0;line[i]!='\0';i++){
            if(line[i]==';'){
                semicolonI = i;
            }
        }
        temp = atof(&line[semicolonI+1]);
        line[semicolonI] = '\0';
        hashKey = hash(line);
        cityCount += insertTemp(hashTable,line,semicolonI+1,hashKey,temp);
    }
    
    // printf("City count is %d\n",cityCount);

    cityArray = (struct City **) malloc(sizeof(struct City*)*cityCount);
    cap = cityArray;
    for(i=0;i<hashSize;i++){
        p = hashTable[i];
        while (p!=NULL){
            p->city->mean_t = p->city->total_t/p->city->count;
            *cap = p->city;
            cap++;
            p = p->next;
        }
    }
    // printf("sorting...\n");
    qsort(cityArray,cityCount,sizeof(struct City*),cityCmp);
    
    cap = cityArray;
    printf("{");
    for(i=0;i<cityCount;i++){
        printf("%s=%.1f/%.1f/%.1f",(*cap)->name,(*cap)->min_t,(*cap)->mean_t,(*cap)->max_t);
        if (i<cityCount-1){
            printf(", ");
        }
        cap++;
    }
    printf("}\n");
    fclose(fp);
    return 0;
}