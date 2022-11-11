#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
/**
 * CSCI 3431: Project 1
 * This program uses Inter Process Communication to calculate the meadian of
 * 25 sorted numbers provide to the program in five different text files.
 *
 * authors: Tushar Mahat (A000429666), Nabin Bhandari (A00430201)
 */

//child number
#define NUMCHILD 5

//# of inputs
#define INPUT_NUMBERS 25

//global declarations
#define REQUEST 100
#define PIVOT  200
#define LARGE  300
#define SMALL  400
#define READY 500
#define KILL 600

//read and write ends and EMPTY signal
#define READ 0
#define WRITE 1
#define EMPTY -1

//variables
int p_c [NUMCHILD] [2]; //parent->child pipes
int c_p [NUMCHILD] [2]; //child->parent pipes

//function prototypes
void createPipes();
void assignChildIDs();
void executeParentProcess();
void executeChildProcess(int i);
void readInputs(int id,char filename[], int list[]);
void sendReadySignalToParent(int i);
void waitForChildrenToBeReady();
int requestPivot(int signal);
int getPivot();
int broadCastToAllChild(int pivot);
void killChildProcess();
void writeSignalToParent(int);
/**
 * Main function of the program
 */
int main(){
    srand(getpid());

	createPipes();// create pipes

    //child 1
	if(fork() == 0){
		executeChildProcess(0);
	}
    //child 2
	else if (fork() == 0) {
		executeChildProcess(1);
	}
    //child 3
	else if (fork() == 0) {
		executeChildProcess(2);
	}
    //child 4
	else if (fork() == 0) {
		executeChildProcess(3);
	}
    //child 5
	else if (fork() == 0) {
		executeChildProcess(4);
	}
    //parent
	else if (fork() > 0){
		executeParentProcess();
	}
	exit(0);
}

/**
 * Executes child process according to the index passed.
 * - reads id from the parent
 * - reads numbers from the input text files
 * - sends READY signal to the parent process
 * - enters while loop
 *      - wait to read signal from parent process
 *      - if signal == REQUEST
 *          -if list is empty sends EMPTY(-1) signal to parent
 *          -else selects random element from the list and writes to 
 *           child->parent pipe
 *      - if singal == PIVOT
 *          -wait to read PIVOT value, count the number of items greater *          than pivot and write to child->parent pipe
 *      - if signal == LARGE 
 *          -delete the elements greater than pivot element
 *      - if signal == SMALL 
 *          -delete the elements smaller than pivot element
 *      - if signal == KILL 
 *          -kill this child process
 * i: index of child process
 */
void executeChildProcess(int i) {
	int id; //the child id number
    char fileName[] = "input_i.txt"; //name of the text file
    int signal; //signal
    int pivot;  //pivot item

    read(p_c[i][READ],&id,sizeof(int)); //read id from the parent->child pipe

    int sizeOfList=5; //size of the list
    int list[sizeOfList]; //list of 5 numbers

	readInputs(id,fileName, list); //read numbers from the input_i.txt

	// printf("%s:  %i %i %i %i %i \n", fileName, list[0], list[1], list[2], list[3], list[4]);

    sendReadySignalToParent(id); //send ready signal to parent

    //enter while loop broken by the parent signal
    while(1){
        read(p_c[id-1][READ],&signal,sizeof(int)); //wait to read signal from parent->child pipe

        //if signal is REQUEST
        if(signal==REQUEST){
            //if list is empty return EMPTY signal
            if(sizeOfList==0){
                signal=EMPTY; //signal for empty list
                write(c_p[id-1][WRITE],&signal,sizeof(int)); //write child->parent pipe
            }
            //if list is not empty send random element
            else{
                int randIndex=(rand() %((sizeOfList-1) - 0 + 1)) + 0;
                int randNum=list[randIndex]; //random element

                //wait to successfully write the random number to child->parent pipe and print message
                if(write(c_p[id-1][WRITE],&randNum,sizeof(int))!=-1){
                    printf("Child %i sends %i to parent\n",id,randNum);
                }
            }
        }
        //if signal is  PIVOT
        else if(signal==PIVOT){
            read(p_c[id-1][READ],&pivot,sizeof(int)); //wait to read pivot element from parent->child pipe

            int m=0; //number of elements greater than pivot element

            //if list not empty count m, else it is 0
            if(sizeOfList!=0){
                for(int i=0;i<sizeOfList;i++){
                    if(list[i]>pivot){
                        m++;
                    }
                }
            }
            //write m to child->parent process
            write(c_p[id-1][WRITE],&m,sizeof(int));
        }
        //if signal is LARGE
        else if (signal == LARGE) {
            int i = 0; //index

            //deletes the elements greater than pivot element
            while (i < sizeOfList)
            { 
                if (list[i] > pivot)
                {
                    if (i < (sizeOfList - 1)){
                        for (int j = i; j < (sizeOfList - 1); j++){
                            list[j] = list[j + 1];
                        }
                    }
                    sizeOfList--; //updates size of list
                }
                else i++;  
            }
        }
        //id the signal is SMALL
        else if (signal == SMALL) {
            int i = 0; //index
            while (i < sizeOfList)
            {
                //deletes the elements smaller than pivot element
                if (list[i] < pivot)
                {
                    if (i < (sizeOfList - 1)){
                        for (int j = i; j < (sizeOfList - 1); j++){
                            list[j] = list[j + 1];
                        }
                    }
                    sizeOfList--; //updates size of list
                }
                else i++;  
            }
        }
        //if signal is KILL
        else if(signal==KILL){
            printf("Child %i terminates\n",id);//print termination message
            exit(0);
        }
    }
}

/**
 * Executes parent process
 * - assigns id to all child processes
 * - waits READY signal from all child processes
 * - enters while loop
 *      -sends REQUEST signal and gets random number from random child and 
 *      sets it to pivot
 *      -broadcasts pivots to all child processes
 *      -if (m==k) meadian found and kill all child processes
 *      -else if(m>k) sends SMALL signal to all child processes
 *      -else if(m<k) sends LARGE signal to all child processes
 */
void executeParentProcess(){
    int m = 0; //m sum of number of elements greater than pivot
	int k = INPUT_NUMBERS / 2; //k=25/2
	int pivot = 0; // pivot is the random element selected from the random child
    int signal; //signal to be sent to child processes

    assignChildIDs(); //assign ids to all child process

    waitForChildrenToBeReady();//wait for children processes to be ready

    printf("Parent READY\n");

    while(1){
        //sends REQUEST signal and gets random number from random child and sets it to pivot
        pivot=getPivot();

        //broadcasts pivots to all child processes
        m=broadCastToAllChild(pivot);

        //meadian found and kill allc child processes
        if (m == k) {
            printf("m:%i = k:%i. Median Found %i\n",m,k, pivot);
            printf("\nParent sends kill signals to all children\n");
            killChildProcess();
            exit(0);
        }
        //sends SMALL signal to all child processes
        else if (m > k) {
            printf("m:%i > k:%i. ",m,k);
            signal=SMALL;
            writeSignalToParent(signal);
        }
        //sends LARGE signal to all child processes
        else if (m < k) {
            printf("m:%i < k:%i. ",m,k);
            signal=LARGE;
            writeSignalToParent(signal);
            k -= m; //updates value of k
        }
    }
}
void writeSignalToParent(int signal){
        for(int i=0;i<NUMCHILD;i++){
            write(p_c[i][WRITE],&signal, sizeof(int));
        }
}
/**
 * Gets pivot from random child
 */
int getPivot(){
        int signal=REQUEST; //REQUEST signal
        int pivot=0; //pivot element
        int selectedChild=(rand() %(NUMCHILD - 1 + 1)) + 1;//select random child

        //wait to successfully write the REQUEST signal to the randomly selected child using parent->child pipe and print message
        if(write(p_c[selectedChild-1][WRITE],&signal, sizeof(int))!=-1){
            printf("\nParent sends REQUEST to child %i\n",selectedChild);
        }
        read(c_p[selectedChild-1][READ],&pivot,sizeof(int));//read pivot from the selected random child using child->parent pipe

        //if pivot is is -1, send request again to a new randomly selected child
        while(pivot==-1){
            printf("No element found in child %i\n",selectedChild);
            selectedChild=(rand() %(NUMCHILD - 1 + 1)) + 1;//select random child

            //wait to successfully write the REQUEST signal to the randomly selected child using parent->child pipe and print message
            if(write(p_c[selectedChild-1][WRITE],&signal, sizeof(int))!=-1){
                printf("Parent sends REQUEST to child %i\n",selectedChild);
            }
            read(c_p[selectedChild-1][READ],&pivot,sizeof(int));//read pivot from the selected random child using child->parent pipe
        }

        return pivot;
}

/**
 * Kills all child process
 */
void killChildProcess(){
    int signal=KILL;//kill signal

    //send kill signal to all child using parent->child pipes
    for(int i=0;i<NUMCHILD;i++){
        write(p_c[i][WRITE],&signal,sizeof(int));
    }
}

/**
 * Broadcasts pivot element to all child processes
 * pivo: selected pivot to be broadcasted to all child
 */
int broadCastToAllChild(int pivot){
        int sum=0; // sum of m array
        int signal=PIVOT; //PIVOT signal
        int m[5]; //number of elements greater than pivots from each child

        //send pivot signal and pivot elements to all child process using parent->child pipes
        for (int i = 0; i < NUMCHILD; i++) {
            write(p_c[i][WRITE],&signal, sizeof(int));//PIVOT signal
            write(p_c[i][WRITE],&pivot, sizeof(int));//pivot number
        }

        printf("Parent Broadcast %i To All Children\n", pivot);
        
        //read numbers greater than pivot from all child using child->parent pipes
        for (int i = 0; i < NUMCHILD; i++) {
            read(c_p[i][READ], &m[i],sizeof(int));
            printf("Child %i receives pivot and replies %i\n", i+1, m[i]);
            sum+=m[i];
        }

        printf("\nParent m = %i+%i+%i+%i+%i=%i. ", m[0], m[1], m[2], m[3], m[4], sum);

        return sum;
}

/**
 * Reads inputs from file and store it in list variable
 * fileName: name of the file
 * list: list to store numbers
 * id: is the id of the child
 */
void readInputs(int id,char fileName[], int list[]) {
	FILE* fp;
    // in filename = "input_i.txt", "i" is replaced by id.
    fileName[6] = '0' + (char)id;//'0'=48, (char)id=1,2,3,4,5
                                 //'0'+(char)id=49,50,51,52,53=>1,2,3,4,5
	fp = fopen(fileName, "r");

    //check if there is error
    if(fp != NULL)
    {
        for (int i = 0; i < NUMCHILD; i++) {
            if (fscanf(fp, "%d", &list[i]) != 1) {
                printf("Error reading file");
                exit(0);
            }
        }
        fclose(fp);
    }
    //show error message
    else{
        printf("Error opening file %s. Press Ctrl+C to exit process.\n",fileName);
        exit(0);
    }
}

/**
 * Writes READY signal to parent using child->parent pipe
 * id: id of child
 */
void sendReadySignalToParent(int id){
    int signal=READY; //ready signal
    write(c_p[id-1][WRITE],&signal,sizeof(int));//write ready signal to child->parent pipe
}

/**
 * Reads READY signal from all child using child->parent pipes
 */
void waitForChildrenToBeReady(){
    int signal; //signal

    //read ready signal from all child processes
    for(int i=0;i<NUMCHILD;i++){
        read(c_p[i][READ],&signal,sizeof(int));//read READY signal from child->parent pipe
        
        //if signal is READY print message
        if(signal == READY){
            printf("Child %i sends READY\n",i+1);
        }
        else{
            exit(0);
        }
    }
}

/**
 * Assigns id from 1-5 to each children
 */
void assignChildIDs(){
    int childID; //child id

    //assign id to all child processes
    for(int i=0;i<NUMCHILD;i++){
        childID=i+1;
        write(p_c[i][WRITE],&childID,sizeof(childID)); // write id to parent->child pipe
    }
}

/**
 * Creates 5 parent->child and 5 child->parent pipes
 */
void createPipes(){
    //create pipes
    for(int i=0;i<NUMCHILD;i++){
        pipe(p_c[i]);
        pipe(c_p[i]);
    }
}