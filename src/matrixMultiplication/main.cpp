/********************************************************************
 * 22I-0485_BS-AI-B_PDC-Assignment-Q1.cpp
 * 
 * Name:        Muhammad Faheem
 * Student ID:  22I-0485
 * Course:      Parallel and Distributed Computing
 * Assignment:  #1
 * Task:        Matrix Computations using Sequential and Multi-threading Techniques
 *
 * Description: This program implements three separate multi-threaded 
 *              applications to process a 1024×1024 matrix using 6 threads:
 *              (a) Determinant of a 6×6 submatrix (using block distribution)
 *              (b) Matrix transposition (using row–wise cyclic distribution)
 *              (c) Element–wise logarithm transformation (using row and column–wise cyclic distribution)
 *              A function CorrectOutputCheck() is implemented (via comparisons)
 *              to verify that the threaded results match the sequential results.
 ********************************************************************/

#include <pthread.h>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace std;

const int MATRIX_SIZE = 1024;  // Full matrix for tasks (transposition, log)
const int DET_SIZE = 6;        // For determinant task (6×6 submatrix)
const int NUM_THREADS = 6;
const double EPSILON = 1e-9;   // Tolerance for floating–point comparisons

// Structures for passing parameters to thread functions

// For the determinant task
struct DetThreadData 
{
    int col;              // Which column in row 0 to process
    const double* matrix; // Pointer to the 6×6 submatrix (in row–major order)
};

// For transposition task
struct TransposeThreadData 
{
    int thread_id, n;  // Matrix dimension (1024)
    const double* input;  // Pointer to input matrix
    double* output;       // Pointer to output (transposed) matrix
};

// For logarithm transformation task
struct LogThreadData 
{
    int thread_id, n;  // Matrix dimension (1024)
    const double* input;  // Pointer to input matrix
    double* output;       // Pointer to output (log-transformed) matrix
};

// Recursive function to compute determinant of an n×n matrix.
// The matrix is passed as a contiguous array in row–major order.
double computeDeterminant(const double* mat, int n) 
{
    if(n == 1)
        return mat[0];
    if(n == 2)
        return mat[0]*mat[3] - mat[1]*mat[2];

    double det = 0.0;

    // Allocatting temporary minor matrix of size (n-1)×(n-1)
    double* minor = new double[(n - 1) * (n - 1)];

    // Expanding along first row (row 0)
    for (int col = 0; col < n; col++) 
    {
        // Building the minor by excluding row 0 and column 'col'
        int minor_i = 0;
    
        for (int i = 1; i < n; i++) 
        {
            int minor_j = 0;
            
            for (int j = 0; j < n; j++) 
            {
                if(j == col)
                    continue;
            
                minor[minor_i * (n - 1) + minor_j] = mat[i * n + j];
            
                minor_j++;
            }
            
            minor_i++;
        }

        double sign = (col % 2 == 0) ? 1.0 : -1.0;
        
        det += sign * mat[col] * computeDeterminant(minor, n - 1);
    }

    delete[] minor;
    
    return det;
}

// Thread function for determinant computation
// Each thread computes one term in the expansion along row 0.
void* determinantThread(void* arg) 
{
    DetThreadData* data = (DetThreadData*) arg;

    int col = data->col;

    const double* matrix = data->matrix; // 6×6 matrix stored in row-major order

    int n = DET_SIZE, minor_size = n - 1; // = 5

    // Building the minor (5×5) by excluding row 0 and column 'col'
    double* minor = new double[minor_size * minor_size];

    int minor_i = 0;
    
    for (int i = 1; i < n; i++) 
    {
        int minor_j = 0;
    
        for (int j = 0; j < n; j++) 
        {
            if(j == col)
                continue;
        
            minor[minor_i * minor_size + minor_j] = matrix[i * n + j];
        
            minor_j++;
        }

        minor_i++;
    }

    double minorDet = computeDeterminant(minor, minor_size);
    
    delete[] minor;
    
    double sign = (col % 2 == 0) ? 1.0 : -1.0;
    
    // Note: matrix[0][col] is at index col in the 1st row.
    double result = sign * matrix[col] * minorDet;

    // Returning the result via heap allocation so that the main thread can sum.
    double* ret = new double;
    
    *ret = result;
    
    pthread_exit(ret);
}

// Thread function for matrix transposition using row-wise cyclic distribution.
void* transposeThread(void* arg) 
{
    TransposeThreadData* data = (TransposeThreadData*) arg;

    int thread_id = data->thread_id, n = data->n;
    const double* input = data->input;
    double* output = data->output;
    
    // Each thread processes rows: thread_id, thread_id+6, thread_id+12, ...
    for (int i = thread_id; i < n; i += NUM_THREADS) 
    {
        for (int j = 0; j < n; j++)
            output[j * n + i] = input[i * n + j];
    }

    pthread_exit(NULL);
}

// Thread function for element-wise logarithm transformation using 
// row and column-wise cyclic distribution.
void* logThread(void* arg) {
    LogThreadData* data = (LogThreadData*) arg;
    int thread_id = data->thread_id;
    int n = data->n;
    int total = n * n;
    const double* input = data->input;
    double* output = data->output;
    // Each thread processes indices: thread_id, thread_id+6, thread_id+12, ...
    for (int index = thread_id; index < total; index += NUM_THREADS) {
        output[index] = std::log(input[index]);
    }
    pthread_exit(NULL);
}

// CorrectOutputCheck: Compares multi-threaded results with sequential ones.
// (Here we check determinant, transposition, and log transformation.)
bool CorrectOutputCheck(double seqDet, double mtDet,
                        const double* seqTranspose, const double* mtTranspose,
                        const double* seqLog, const double* mtLog,
                        int n) 
{
    bool correct = true;

    // Checking determinant
    if (fabs(seqDet - mtDet) > EPSILON) 
    {
        cout << "Determinant mismatch: sequential " << seqDet
             << ", multithreaded " << mtDet << endl;
    
        correct = false;
    }
    
    // Checking transposition
    for (int i = 0; i < n * n; i++) 
    {
        if (fabs(seqTranspose[i] - mtTranspose[i]) > EPSILON) 
        {
            cout << "Transpose mismatch at index " << i
                 << ": sequential " << seqTranspose[i]
                 << ", multithreaded " << mtTranspose[i] << endl;
            correct = false;
        
            break;
        }
    }

    // Checking log transformation
    for (int i = 0; i < n * n; i++) 
    {
        if (fabs(seqLog[i] - mtLog[i]) > EPSILON) 
        {
            cout << "Log transformation mismatch at index " << i
                 << ": sequential " << seqLog[i]
                 << ", multithreaded " << mtLog[i] << endl;
            correct = false;
        
            break;
        }
    }

    return correct;
}

// Utility function
void DisplayMatrix(double* matrix, int n, int display_size = 10) 
{
    cout << "\n*Displaying a portion of the matrix:" << endl;

    // Limiting the displayed matrix size to avoid excessive output
    int rows_to_display = (n > display_size) ? display_size : n;
    int cols_to_display = (n > display_size) ? display_size : n;

    for (int i = 0; i < rows_to_display; i++) 
    {
        for (int j = 0; j < cols_to_display; j++)
            cout << matrix[i * n + j] << "\t";
        cout << endl;
    }
}

// Driver function
int main() {
    // 1. Initializing the 1024×1024 matrix.
    int n = MATRIX_SIZE;
    double* matrix = new double[n * n];
    
    // // Filling matrix with positive values; here: matrix[i][j] = i*n + j + 1.
    // for (int i = 0; i < n; i++) 
    // {
    //     for (int j = 0; j < n; j++)
    //         matrix[i * n + j] = i * n + j + 1;
    // }

    // Seed for random number generation
    srand(time(0));

    // Filling matrix with random positive values in range (0, 1000)
    for (int i = 0; i < n; i++) 
    {
        for (int j = 0; j < n; j++)
            matrix[i * n + j] = (rand() % 1000) + 1; // Values in range [1, 1000]
    }

    cout << "> Matrix initialized with random values in between range [1, 1000]" << endl;

    DisplayMatrix(matrix, n);

    // 2. Extracting a 6×6 submatrix (top-left) for the determinant task.
    double* detMatrix = new double[DET_SIZE * DET_SIZE];
    
    for (int i = 0; i < DET_SIZE; i++) 
    {
        for (int j = 0; j < DET_SIZE; j++)
            detMatrix[i * DET_SIZE + j] = matrix[i * n + j];
    }

    cout << "\n> Extracted 6x6 submatrix for determinant computation" << endl;

    // 3. Sequential (golden) computations.
    // (a) Sequential Determinant of the 6×6 submatrix.
    double seqDet = computeDeterminant(detMatrix, DET_SIZE);

    cout << ">> Sequential determinant computation completed" << endl;

    // (b) Sequential Matrix Transposition.
    double* seqTranspose = new double[n * n];

    for (int i = 0; i < n; i++) 
    {
        for (int j = 0; j < n; j++)
            seqTranspose[j * n + i] = matrix[i * n + j];
    }

    cout << ">> Sequential matrix transposition completed" << endl;

    // (c) Sequential Element-wise Log Transformation.
    double* seqLog = new double[n * n];

    for (int i = 0; i < n * n; i++)
        seqLog[i] = log(matrix[i]);

    cout << ">> Sequential log tranformation completed" << endl;

    cout << "\n> Sequential computations completed." << endl;

    // 4. Multi-threaded computations.
    // (a) Determinant using 6 threads.
    pthread_t detThreads[NUM_THREADS];

    DetThreadData detData[NUM_THREADS];

    double mtDet = 0.0;

    for (int col = 0; col < NUM_THREADS; col++) 
    {
        detData[col].col = col;
        detData[col].matrix = detMatrix;
    
        int rc = pthread_create(&detThreads[col], NULL, determinantThread, (void*) &detData[col]);
    
        if(rc) 
        {
            cerr << "Error: unable to create determinant thread, " << rc << endl;
        
            exit(-1);
        }
    }

    // Joining determinant threads and sum their contributions.
    for (int col = 0; col < NUM_THREADS; col++) 
    {
        void* ret;
    
        pthread_join(detThreads[col], &ret);
    
        double* threadResult = (double*) ret;
    
        mtDet += *threadResult;
    
        delete threadResult;
    }

    cout << ">> Multi-threaded determinant computation completed" << endl;

    // (b) Matrix Transposition using 6 threads.
    double* mtTranspose = new double[n * n]; // Output buffer for transposition.
    
    pthread_t transThreads[NUM_THREADS];
    
    TransposeThreadData transData[NUM_THREADS];
    
    for (int t = 0; t < NUM_THREADS; t++) 
    {
        transData[t].thread_id = t;
        transData[t].n = n;
        transData[t].input = matrix;
        transData[t].output = mtTranspose;
    
        int rc = pthread_create(&transThreads[t], NULL, transposeThread, (void*) &transData[t]);
    
        if(rc) 
        {
            cerr << "Error: unable to create transpose thread, " << rc << endl;
        
            exit(-1);
        }
    }

    for (int t = 0; t < NUM_THREADS; t++)
        pthread_join(transThreads[t], NULL);

    cout << ">> Multi-threaded matrix transposition completed" << endl;

    // (c) Element-wise Log Transformation using 6 threads.
    double* mtLog = new double[n * n]; // Output buffer for log transformation.
    
    pthread_t logThreads[NUM_THREADS];
    
    LogThreadData logData[NUM_THREADS];
    
    for (int t = 0; t < NUM_THREADS; t++) 
    {
        logData[t].thread_id = t;
        logData[t].n = n;
        logData[t].input = matrix;
        logData[t].output = mtLog;
    
        int rc = pthread_create(&logThreads[t], NULL, logThread, (void*) &logData[t]);
    
        if(rc) 
        {
            cerr << "Error: unable to create log thread, " << rc << endl;
        
            exit(-1);
        }
    }

    for (int t = 0; t < NUM_THREADS; t++)
        pthread_join(logThreads[t], NULL);

    cout << ">> Multi-threaded log tranformation completed" << endl;

    cout << "\n> Multi-threaded computations completed." << endl;

    cout << "\n> Verifications:" << endl;

    // 5. Verification: Compare multi-threaded vs. sequential outputs.
    bool correct = CorrectOutputCheck(seqDet, mtDet, seqTranspose, mtTranspose, seqLog, mtLog, n);

    if (correct)
        cout << ">> CorrectOutputCheck: All multi-threaded computations are correct." << endl;
    else
        cout << ">> CorrectOutputCheck: MISMATCH found in multi-threaded results." << endl;

    // 6. (Optional) Display a summary of the determinant result.
    cout << ">> Sequential Determinant = " << seqDet << endl;
    cout << ">> Multi-threaded Determinant = " << mtDet << endl;

    // 7. Cleanup dynamic memory.
    delete[] matrix;
    delete[] detMatrix;
    delete[] seqTranspose;
    delete[] seqLog;
    delete[] mtTranspose;
    delete[] mtLog;

    return 0;
}
