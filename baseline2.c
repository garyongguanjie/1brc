#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define hashSize 1024
#define lineSize 256
#define bufferSize 8192

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
    int lineP;
    char buffer[bufferSize];
    int bufferEnd;
    int semicolonI;
    int dot;
    double sign;
    int i;
    unsigned hashKey;
    int readCity;
    double temp;
    struct City **cityArray;
    struct City **cap;
    struct Node *p;
    
    FILE *fp = fopen("measurements.txt","r");

    if (fp==NULL){
        printf("Cannot read file\n");
        return 1;
    }
    
    readCity = 1;
    lineP = 0;
    temp = 0;
    dot = 0;
    sign = 1;
    while ((bufferEnd=fread(buffer,sizeof(char),bufferSize,fp))){
        for (i=0;i<bufferEnd;i++){
            if (buffer[i]==';'){
                line[lineP] = '\0';
                readCity = 0;
                continue;
            }
            if (buffer[i]=='\n'){
                // fine because final line ends with \n
                temp*=sign;
                hashKey = hash(line);
                // printf("%s %.1f\n",line,temp);
                cityCount += insertTemp(hashTable,line,lineP+1,hashKey,temp);
                readCity = 1;
                lineP = 0;
                temp = 0;
                dot = 0;
                sign = 1;
                continue;
            }
            if (readCity){
                line[lineP++] = buffer[i];
            }else if(buffer[i]=='.'){
                dot = 1;
            }else if (dot){
                temp += ((double)(buffer[i] - '0'))/10;
            }else{
                if (isdigit(buffer[i])){
                    temp*=10;
                    temp+= buffer[i] - '0';
                }else if (buffer[i]=='-'){
                    // printf("reached\n");
                    sign = -1;
                }else{
                    printf("error");
                }

            }
            
        }
    }


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