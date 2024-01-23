//Luis Eduardo Exposto Novoselecki  11208200

#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


/* Globals set by command line args */
int verbosity = 0; /* print trace if set */
int s = 0; /* set index bits */
int b = 0; /* block offset bits */
int E = 0; /* associativity */
FILE *trace_file;   
int hit_count = 0;
int miss_count = 0;
int eviction_count;

/*
 * printUsage - Print usage info
 */

typedef struct cacheline {
    int valid;
    unsigned int tag;
    int used;  // usado a ultima vez
    int first; //se é o primeiro acesso ao cache
} CacheLine;


// ultimo set usado recentemente de sua linha
int LRU(CacheLine **cache, int sets, int lines, int Set) {
	int index = 0;
    int max = 0;

	for (int i = 0; i < lines; i++) {
		if ((cache[Set][i]).used > max) {
			max = (cache[Set][i]).used;
			index = i;
		}
	}

	return index;
}

//update nos sets utilizados recentementes
void updateUsed(CacheLine **cache, int sets, int lines, int Set, int Line) {

		for (int j = 0; j < lines; j++) {
				(cache[Set][j]).used += 1; 

		}

    (cache[Set][Line]).used = 0; 
}

//inicializa o cache
void initialize(CacheLine **cache, int sets, int lines){
    for(int i = 0; i< sets;i++){
	    for (int j = 0; j < lines; j++) {
		    cache[i][j].tag = 0;
            cache[i][j].valid = 1;
            cache[i][j].used = 0;
            cache[i][j].first = 1;
	    }
    }
}

void printUsage(char* argv[])
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

int main(int argc, char* argv[])
{
    char c;
    char *file;
    while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1){
        switch(c){
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            file = optarg;
            break;
        case 'v':
            verbosity = 1;
            break;
        case 'h':
            printUsage(argv);
            exit(0);
        default:
            printUsage(argv);
            exit(1);
        }
    }

    int sets = 1 << s; // 2**s
    struct cacheline **cache; 
    trace_file = fopen(file , "r");

    cache = malloc(sets*sizeof(struct cacheline*));

	char line[32]; // cada linha do trace  

    for (int i = 0; i < sets; i++) {
		cache[i] = malloc( E*sizeof(struct cacheline) );
	}

    initialize(cache, sets, E);

    	while(fgets(line, 32, trace_file) != NULL) { 


		int hit = 0;  // booleano
        int evict = 0;
        int lru = 0; // 
        int valid = -1; //espaco vazio no cache
        

		// ignorar comando de instrucao
		if (line[0] == ' ') {

            char *operation = strtok(line, ",");
	        char *size = strtok (NULL, " ");  
            operation = strtok(operation, " "); // tipo de operacao
            char *address = strtok(NULL, " "); 

			if (verbosity) printf("%s %s , %s ", operation, address, size); 

			unsigned int addr = strtol(address, NULL, 16); 
            unsigned int tag_size = 32 - b - s;
			unsigned int tag = addr >> (32-tag_size); // tag
			unsigned int set = addr << (tag_size); //set
            set = set >> (tag_size + b);

			// buscar tag no set do cache
			for (int i = 0; i < E; i++) {
                //caso encontrar
				if ( cache[set][i].valid == 1 && cache[set][i].tag == tag  && cache[set][i].first == 0 ) {
					hit = 1; 
					updateUsed(cache, sets, E, set, i); // atualiza as ultimas linhas usadas
					break;
				}

				// se ha espaco vazio no cache, utilizar
				else if (cache[set][i].first == 1) { 
					valid = i; 
				}
			}

			if (!hit) {

				// se ha espaco livre
				if (valid != -1) {
					evict = 0;
					cache[set][valid].valid = 1; 
                    cache[set][valid].tag = tag;
					updateUsed(cache, sets, E, set, valid); // update used lines in cache (For LRU)
                    cache[set][valid].first = 0; // ja utilizado  pelo cache
				}

				// nao ha espaco livre
				else {
					evict = 1;
					lru = LRU(cache, sets, E, set);
					(cache[set][lru]).valid = 1; 
					(cache[set][lru]).tag = tag; 
					updateUsed(cache, sets, E, set, lru); // update used lines in cache (For LRU)
				}
			}

             
            if(hit){
                hit_count ++;
                if(verbosity) printf(" hit");
            }
            else if(evict){
                miss_count ++;
                eviction_count++;
                if(verbosity) printf(" miss eviction");
            }
            else{
                miss_count++;
                if(verbosity) printf(" miss");
            }
            
            if(*operation == 'M'){
                hit_count ++;
                if(verbosity) printf(" hit");
            }

			
			
		}

    printf("\n");
 }


 /***************
	* Print summary, close file and clean memory
	**************/

	printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
