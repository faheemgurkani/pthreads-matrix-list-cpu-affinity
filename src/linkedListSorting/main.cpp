/********************************************************************
 * 22I-0485_BS-AI-B_PDC-Assignment-Q2.cpp
 * 
 * Name:        Muhammad Faheem
 * Student ID:  22I-0485
 * Course:      Parallel and Distributed Computing
 * Assignment:  #1
 * Task:        Sequential and Multi-threaded Sorting
 *
 * Description: 
 *   This program implements two versions (serial and parallel) to:
 *     1. Read a list of roll numbers from a file into an array.
 *     2. Build a linked list containing those numbers.
 *     3. Sort the linked list using a quick sort algorithm.
 *
 *   The parallel version uses Pthreads to:
 *     - Insert numbers concurrently into the linked list (using a mutex
 *       to avoid race conditions).
 *     - Sort the linked list in parallel by sorting partitions concurrently.
 *     - Set CPU affinity for each thread using pthread_setaffinity_np.
 *
 *   A thread–mapping plan is demonstrated by assigning specific cores to 
 *   insertion and sorting threads.
 ********************************************************************/

#include <pthread.h>
#include <sched.h>      // For CPU affinity functions
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <ctime>
#include <chrono>

using namespace std;


// Definition of the Node structure
struct Node 
{
    int data;

    Node* next;
};

void setAffinity(pthread_t thread, int coreId);

// -----------------------------
// Serial Version Functions
// -----------------------------

// (i) Reading a list of numbers from a file and store them in an array.
void readRollNumbers(FILE* inputFile, int* Numbers, int num) 
{
    for (int i = 0; i < num; i++)
        fscanf(inputFile, "%d", &Numbers[i]);
}

// (ii) Inserting the numbers into a linked list (appending at the end).
void addRollNumbersToList(Node** head, int* Numbers, int num) 
{
    for (int i = 0; i < num; i++) 
    {
        Node* newNode = new Node;
    
        newNode->data = Numbers[i];
        newNode->next = NULL;
    
        if (*head == NULL)
            *head = newNode;
        else 
        {
            Node* temp = *head;
        
            while (temp->next != NULL)
                temp = temp->next;
        
            temp->next = newNode;
        }
    }
}

// (iii) Quick sort for linked list (serial version).
// This implementation partitions the list into three parts (less, equal, greater)
// and then recursively sorts the "less" and "greater" lists.
Node* quickSort(Node* head) 
{
    if (!head || !head->next)
         return head;

    int pivot = head->data;

    Node* less = NULL;
    Node* equal = NULL;
    Node* greater = NULL;
    Node* lessTail = NULL;
    Node* equalTail = NULL;
    Node* greaterTail = NULL;
    Node* current = head;
    
    while (current) 
    {
         Node* next = current->next;
    
         current->next = NULL;

         if (current->data < pivot) 
         {
              if (!less) 
              {
                   less = current;
                   lessTail = current;
              } 
              else 
              {
                   lessTail->next = current;
                   lessTail = current;
              }
         } 
         else if (current->data == pivot) 
         {
              if (!equal) 
              {
                   equal = current;
                   equalTail = current;
              } 
              else 
              {
                   equalTail->next = current;
                   equalTail = current;
              }
         } 
         else 
         {
              if (!greater) 
              {
                   greater = current;
                   greaterTail = current;
              } 
              else 
              {
                   greaterTail->next = current;
                   greaterTail = current;
              }
         }

         current = next;
    }

    less = quickSort(less);
    greater = quickSort(greater);
    
    // Merging the sorted lists: less -> equal -> greater.
    Node* result = NULL;
    Node* tail = NULL;

    if (less) 
    {
         result = less;
         tail = less;
    
         while (tail->next)
              tail = tail->next;
    
         tail->next = equal;
    } 
    else 
    {
         result = equal;
         tail = equal;
    }

    if (tail) 
    {
         while (tail->next)
              tail = tail->next;
    
         tail->next = greater;
    } 
    else 
         result = greater;

    return result;
}

// -----------------------------
// Parallel Version Functions
// -----------------------------

Node* parallelHead = NULL;  // Global pointer for the linked list built concurrently.
pthread_mutex_t listMutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex to protect concurrent insertions.

// Structure to pass a subset of the numbers to an insertion thread.
struct ParallelInsertData 
{
    int* numbers;
    int start;
    int end;  // end index (non-inclusive)
};

// Thread function: Insert a subset of numbers into the global linked list.
void* addRollNumbersToListParallel(void* arg) 
{
    ParallelInsertData* data = (ParallelInsertData*) arg;

    for (int i = data->start; i < data->end; i++) 
    {
         Node* newNode = new Node;
         newNode->data = data->numbers[i];
    
         // For speed, insert at the head.
         pthread_mutex_lock(&listMutex);
    
         newNode->next = parallelHead;
         parallelHead = newNode;
    
         pthread_mutex_unlock(&listMutex);
    }
    
    pthread_exit(NULL);
}

// Utility function: Count the number of nodes in a linked list.
int countNodes(Node* head) 
{
    int count = 0;

    while(head) 
    {
         count++;
         head = head->next;
    }

    return count;
}

// Forward declaration for the quick sort thread function.
void* quickSortParallel(void* arg);

// Utility function: Recursively perform parallel quick sort on the list.
// For small lists (fewer than 10 nodes), the serial quickSort is used.
Node* quickSortParallelUtil(Node* head) 
{
    if (!head || !head->next)
         return head;

    if (countNodes(head) < 10) // Threshold to reduce threading overhead.
         return quickSort(head);
    
    int pivot = head->data;

    Node* less = NULL;
    Node* equal = NULL;
    Node* greater = NULL;
    Node* lessTail = NULL;
    Node* equalTail = NULL;
    Node* greaterTail = NULL;
    Node* current = head;
    
    // Partition the list into three parts.
    while (current) 
    {
         Node* next = current->next;
    
         current->next = NULL;
    
         if (current->data < pivot) 
         {
              if (!less) 
              {
                   less = current;
                   lessTail = current;
              } 
              else 
              {
                   lessTail->next = current;
                   lessTail = current;
              }
         } 
         else if (current->data == pivot) 
         {
              if (!equal) 
              {
                   equal = current;
                   equalTail = current;
              } 
              else 
              {
                   equalTail->next = current;
                   equalTail = current;
              }
         } 
         else 
         {
              if (!greater) 
              {
                   greater = current;
                   greaterTail = current;
              } 
              else 
              {
                   greaterTail->next = current;
                   greaterTail = current;
              }
         }

         current = next;
    }
    
    pthread_t threadLess, threadGreater;
    
    Node* sortedLess = NULL;
    Node* sortedGreater = NULL;

    bool spawnLess = (less != NULL);
    bool spawnGreater = (greater != NULL);
    
    // Spawnning threads to sort the "less" and "greater" lists concurrently.
    if (spawnLess) 
    {
         pthread_create(&threadLess, NULL, quickSortParallel, (void*) less);
    
         // For demonstration, assign threadLess to core 1.
         setAffinity(threadLess, 1);
    }
    if (spawnGreater) 
    {
         pthread_create(&threadGreater, NULL, quickSortParallel, (void*) greater);
    
         // For demonstration, assign threadGreater to core 2.
         setAffinity(threadGreater, 2);
    }
    if (spawnLess) 
    {
         void* ret;
    
         pthread_join(threadLess, &ret);
    
         sortedLess = (Node*) ret;
    }
    if (spawnGreater) 
    {
         void* ret;
    
         pthread_join(threadGreater, &ret);
    
         sortedGreater = (Node*) ret;
    }
    
    // Merging the sorted partitions: sortedLess -> equal -> sortedGreater.
    Node* result = NULL;
    Node* tail = NULL;

    if (sortedLess) 
    {
         result = sortedLess;
         tail = sortedLess;
    
         while(tail->next)
              tail = tail->next;
    
         tail->next = equal;
    } 
    else 
    {
         result = equal;
         tail = equal;
    }

    if (tail) 
    {
         while(tail->next)
              tail = tail->next;
    
         tail->next = sortedGreater;
    } 
    else 
         result = sortedGreater;
    
    return result;
}

// Thread function for parallel quick sort.
// It simply calls the utility function and returns the sorted list.
void* quickSortParallel(void* arg) 
{
    Node* head = (Node*) arg;
    Node* sorted = quickSortParallelUtil(head);

    pthread_exit((void*) sorted);
}

// -----------------------------
// CPU Affinity Helper Function
// -----------------------------
// Binds the given thread to the specified core.
void setAffinity(pthread_t thread, int coreId) 
{
    cpu_set_t cpuset;
    
    CPU_ZERO(&cpuset);             // Clearing the CPU set
    CPU_SET(coreId, &cpuset);      // Setting the desired CPU core

    // Applying the CPU affinity settings to the thread
    int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    
    if (result != 0) 
        cerr << "Error setting thread affinity to core " << coreId << endl;
}

// Utility function: Measures and prints execution times for serial and parallel versions.
void runPerformanceTests(const char* filename, int num, bool setAffinityFlag) 
{
    // Allocating array to hold roll numbers.
    int* numbers = new int[num];
    
    // Opening file and read numbers.
    FILE* fp = fopen(filename, "r");

    if (!fp) 
    {
        cerr << "Error opening file " << filename << endl;
    
        delete[] numbers;
    
        return;
    }

    readRollNumbers(fp, numbers, num);
    
    fclose(fp);
    
    // ----------- Serial Version Timing -----------
    clock_t startSerial = clock();
    
    // Building linked list using serial insertion.
    Node* serialHead = NULL;
    
    addRollNumbersToList(&serialHead, numbers, num);
    
    // Sorting the list using serial quick sort.
    Node* sortedSerial = quickSort(serialHead);
    
    clock_t endSerial = clock();
    
    double serialTime = double(endSerial - startSerial) / CLOCKS_PER_SEC;
    
    // Freeing the serial sorted list.
    Node* temp = sortedSerial;
    
    while (temp) 
    {
        Node* next = temp->next;
    
        delete temp;
    
        temp = next;
    }
    
    // ----------- Parallel Version Timing -----------
    // Resetting the global linked list for parallel insertion.
    parallelHead = NULL;
    
    // Launching parallel insertion threads.
    int numThreads = 4;  // Example: using 4 threads for insertion.

    pthread_t insertThreads[numThreads];
    
    ParallelInsertData insertData[numThreads];
    
    int chunkSize = num / numThreads;
    
    for (int i = 0; i < numThreads; i++) 
    {
        insertData[i].numbers = numbers;
        insertData[i].start = i * chunkSize;
    
        if (i == numThreads - 1)
            insertData[i].end = num;
        else
            insertData[i].end = (i + 1) * chunkSize;
        
        pthread_create(&insertThreads[i], NULL, addRollNumbersToListParallel, (void*) &insertData[i]);
    
        if (setAffinityFlag)
            setAffinity(insertThreads[i], i);       // Setting CPU affinity as in your original code.
    }
    
    // Wait for all insertion threads to complete.
    for (int i = 0; i < numThreads; i++) {
        pthread_join(insertThreads[i], NULL);
    }
    
    // Start timing for parallel quick sort.
    clock_t startParallel = clock();
    
    pthread_t sortThread;
    pthread_create(&sortThread, NULL, quickSortParallel, (void*) parallelHead);
    
    if (setAffinityFlag)
        setAffinity(sortThread, 0); // Binding the sorting thread to a specific core (as in your code).
    
    void* ret;
    
    pthread_join(sortThread, &ret);
    
    Node* sortedParallel = (Node*) ret;
    
    clock_t endParallel = clock();
    
    double parallelTime = double(endParallel - startParallel) / CLOCKS_PER_SEC;
    
    // Freeing the parallel sorted list.
    temp = sortedParallel;
    
    while (temp) 
    {
        Node* next = temp->next;
    
        delete temp;
    
        temp = next;
    }
    
    // Freeing the numbers array.
    delete[] numbers;
    
    cout << ">> Serial execution time: " << serialTime << " seconds." << endl;
    cout << ">> Parallel execution time: " << parallelTime << " seconds." << endl;
}

// -----------------------------
// Driver Function
// -----------------------------
int main() 
{
    // For demonstration, we use a small input size.
    const int num = 20;
    int* numbers = new int[num];
    
    // Openning the file "sampleRollnumbers.txt" for reading.
    FILE* fp = fopen("sampleRollNumbers.txt", "r");

    if (!fp) 
    {
         cerr << "Error opening file sampleRollnumbers.txt" << endl;
    
         return -1;
    }

    readRollNumbers(fp, numbers, num);
    
    fclose(fp);

    cout << "> File loading successfull" << endl;
    
    // ------------------ Serial Version ------------------
    Node* serialHead = NULL;

    addRollNumbersToList(&serialHead, numbers, num);
    
    // Sorting the list using the serial quick sort.
    Node* sortedSerial = quickSort(serialHead);
    
    cout << "\n> Serial sorted list:" << endl;
    Node* temp = sortedSerial;
    
    while (temp) 
    {
         cout << temp->data << " ";
         temp = temp->next;
    }
    cout << endl;
    
    // Freeing the serial sorted list.
    temp = sortedSerial;

    while (temp) {
         Node* next = temp->next;
    
         delete temp;
    
         temp = next;
    }

    cout << "\n>> Serial version completed" << endl;
    
    // ------------------ Parallel Version ------------------
    // Resetting the global linked list for parallel insertion.
    parallelHead = NULL;
    
    // Using 4 threads for concurrent insertion.
    int numThreads = 4;

    pthread_t insertThreads[numThreads];
    
    ParallelInsertData insertData[numThreads];
    
    int chunkSize = num / numThreads;
    
    for (int i = 0; i < numThreads; i++) 
    {
         insertData[i].numbers = numbers;
         insertData[i].start = i * chunkSize;
    
         if (i == numThreads - 1)
              insertData[i].end = num;
         else
              insertData[i].end = (i + 1) * chunkSize;
    
         pthread_create(&insertThreads[i], NULL, addRollNumbersToListParallel, (void*) &insertData[i]);
    
         // Mapping each insertion thread to a specific core (e.g., core = i).
         setAffinity(insertThreads[i], i);
    }

    for (int i = 0; i < numThreads; i++)
         pthread_join(insertThreads[i], NULL);
    
    // Now performing parallel quick sort on the concurrently built linked list.
    pthread_t sortThread;

    pthread_create(&sortThread, NULL, quickSortParallel, (void*) parallelHead);
    
    // For demonstration, bind the sorting thread to core 0.
    setAffinity(sortThread, 0);
    
    Node* sortedParallel = NULL;
    
    void* ret;
    
    pthread_join(sortThread, &ret);
    
    sortedParallel = (Node*) ret;
    
    cout << "\n> Parallel sorted list:" << endl;
    
    temp = sortedParallel;
    
    while (temp) 
    {
         cout << temp->data << " ";
         temp = temp->next;
    }
    cout << endl;
    
    // Freeing the parallel sorted list.
    temp = sortedParallel;
    
    while (temp) 
    {
         Node* next = temp->next;
         
         delete temp;
         
         temp = next;
    }

    cout << "\n>> Parallel version completed" << endl;
    
    delete[] numbers;

    const char* filename = "sampleRollNumbers.txt";
    
    int n = 1000; 

    cout << "\n> Performance Testing:" << endl;

    runPerformanceTests(filename, n, true);
    
    return 0;
}
