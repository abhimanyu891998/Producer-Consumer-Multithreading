/*
File Name : thrwordcnt.c
Student's Name: Abhimanyue Singh Tanwar
Student Number: 3035392177
Development Platform: Ubuntu 17.10
Compilation: ./thrwordcnt [number of workers] [number of buffers] [target plaintext file] [keyword file]
Remark: To my knowledge, all done :)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/resource.h>

#define MAXLEN	116

//align to 8-byte boundary
struct output {
	char word[MAXLEN];
	unsigned int count;
};


sem_t full;           /* keep track of the number of full spots */
sem_t empty;          /* keep track of the number of empty spots */
sem_t mutex;          /* allows mutual exclusion in critical sections  */

int noOfWorkerThreads; //Total number of worker threads
int buffSize; //Size of buffer
char * target; //Name of target file is stored here
int lines; //Total number of keywords to be searched
int head=0; //Head of the queue to extract in FIFO order
int tail=0; //Head of the queue to extract in FIFO order
int counter = 0; //To keep a check on the number of keywords already checked
struct output * result; //Final result array
struct output * buffer; //bounded_buffer


char * lower(char * input){
	unsigned int i;	
	for (i = 0; i < strlen(input); ++i)
		input[i] = tolower(input[i]);
	return input;
}


void *Consumer(void *arg); //Function definition for consumer
unsigned int search(char * keyword); //Function definition for search



int main(int argc, char * argv[]){
    
    //Condition to check for irregular input
    if(argc<5){
        printf("Usage: ./thrwordcnt [number of workers] [number of buffers] [target plaintext file] [keyword file]\n");
		return 0;
    }

    noOfWorkerThreads = atoi(argv[1]);
    if(noOfWorkerThreads<=0 || noOfWorkerThreads>15){
        printf("The number of worker threads must be between 1 and 15. \n");
        return 0;
    }
    buffSize = atoi(argv[2]);
    if(buffSize<=0 || buffSize>10 ){
        printf("The number of buffer in task pool must be between 1 to 10.\n");
        return 0;
    }
    target = strdup(argv[3]);
    
    char word[MAXLEN];
    pthread_t idC[noOfWorkerThreads];
    FILE * k;
    k = fopen(argv[4], "r");
	fscanf(k, "%d", &lines);
    
    //allocate space to store the results
	result = (struct output *) malloc(lines * sizeof(struct output));
    
    //perform the search
	for (int i = 0; i < lines; ++i) {
        fscanf(k, "%s", word);
		result[i].count =0;
		strcpy(result[i].word, word);
    }
	fclose(k);
  
    sem_init(&full, 0, 0);//initializing full to zero so that consumer thread knows buffer is empty and waits for producer thread
    sem_init(&empty, 0, buffSize);//initializing empty to buffer size so that producer knows to add that many tasks
    sem_init(&mutex, 0,1);//initalizing mutex to serve as lock for concurrency 

    //making threads here
    for(int index=0; index<noOfWorkerThreads; index++)
    {
		printf("Worker(%d): Start up. Wait for task!\n", index); 
        pthread_create(&idC[index], NULL, Consumer, (void*) &index);//creating worker threads to go to consumer function sending their index
		sleep(1);
    }
    
    
    int lineNo=0;
    
    buffer = (struct output *) malloc (sizeof(struct output) * buffSize);
    //Filling up the buffer by the producer
    while(lineNo<lines){
        
        char word[MAXLEN];
        strcpy(word, result[lineNo].word);

        //critical section in master thread where it starts serving as the producer
        sem_wait(&empty);
        sem_wait(&mutex); //Acquires lock

        //populate buffer with this word
        strcpy(buffer[tail].word,word);
        buffer[tail].count=0;
        tail=(tail+1)% buffSize;
        
        sem_post(&mutex); //Releases lock
        sem_post(&full);
        lineNo++;
        
    }
    
    //Prinitng result
    for(int threadNumber=0;threadNumber<noOfWorkerThreads;threadNumber++){
        pthread_join(idC[threadNumber], NULL);
    }

    printf("All worker threads have terminated \n");
    
    //print result
    for (int i = 0; i <lines; ++i)
		printf("%s : %d\n", result[i].word, result[i].count);
    
   	free(target);
	free(result);
    
	return 0;

}


//function for consumer threads to accesss
void *Consumer(void *arg)
{	
	char processingKeyword[MAXLEN]; //Current processing keyword
	int currentThredTasks = 0; //Counter to check for number of tasks completed by current thread
    int index; //To store the index of current index
    index= *(int *) arg; //initializing the thread index

    for(int i=0 ; i<lines ; i++){
        
        if(counter==lines) {
            sem_post(&full);
            break;
        }
        
        sem_wait(&full);
        sem_wait(&mutex); //Acquiring the lock
       
    
        //Processing the word
        if(counter<lines){
            strcpy(processingKeyword,buffer[head].word);
            printf("Worker(%d): search for keyword \"%s\" \n", index,processingKeyword); 
            int countVal = search(processingKeyword);
            result[counter].count = countVal;
            head = (head+1)%buffSize;
            counter++;
            currentThredTasks++;
        }

        sem_post(&mutex); //Releases the lock
        sem_post(&empty);//informing a producer that a task has been taken out and if tasks are left buffer is empty to add tasks

    }

    printf("Worker %d has terminated and completed %d tasks. \n",index,currentThredTasks);

    return 0;
}



//Function to return the count of keyword in target file (copied from template)
unsigned int search(char * keyword){
	int count = 0;
	FILE * f;
	char * pos;
	char input[MAXLEN];
	char * word = strdup(keyword);		//clone the keyword	

	lower(word);	//convert the word to lower case
	f = fopen(target, "r");		//open the file stream

    while (fscanf(f, "%s", input) != EOF){  //if not EOF
        lower(input);	//change the input to lower case
        if (strcmp(input, word) == 0)	//perfect match
			count++;
		else {
			pos = strstr(input, word); //search for substring
			if (pos != NULL) { //keyword is a substring of this string
				//if the character before keyword in this input string
				//is an alphabet, skip the checking
				if ((pos - input > 0) && (isalpha(*(pos-1)))) continue; 
				//if the character after keyword in this input string
				//is an alphabet, skip the checking
				if (((pos-input+strlen(word) < strlen(input))) 
					&& (isalpha(*(pos+strlen(word))))) continue;
				//Well, we count this keyword as the characters before 
				//and after the keyword in this input string are non-alphabets
				count++;  
			}
		}
    }
    fclose(f);
	free(word);
	
    return count;
} 
