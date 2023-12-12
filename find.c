#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>


#define MAXWORDSIZE 8192

unsigned int filelength(FILE* fp) {
  unsigned int size;
  fseek(fp, 0L, SEEK_END);  // Go to the end of the file
  size = ftell(fp);         // Get the current position
  fseek(fp, 0L, SEEK_SET);  // Go to the start of the file
  return size;
}

typedef struct {  //This struct is being passed to each thred and contains all the necessary information
    FILE* file;
    char* filename;
    char* word;
    int start;
    int end;
} ThreadData;

void* countWord(void* arg) {
    int chunks = 0;
    ThreadData* data = (ThreadData*)arg; // Struct is being assigned 
    int count = 0;
    char buffer[MAXWORDSIZE];
    size_t word_length = strlen(data->word);

    FILE* file = fopen(data->filename, "rb");
    fseek(file, data->start, SEEK_SET); // File pointer being set to the start of the block

    //printf("starting at %d and ending at %d\n", data->start, data->end);

    fseek(file, data->start, SEEK_SET);

    while (ftell(file) < data->end) {
        chunks++;
        int buffersize = sizeof(buffer);

        long currentpos = ftell(file);
        if (currentpos + sizeof(buffer) > data->end) //If block that is being read exceeds the block that the thread is supposed to read, size is adjusted 
            buffersize = data->end - currentpos;
            
        size_t read_bytes = fread(buffer, sizeof(char), buffersize, file);

        for (size_t i = 0; i <= read_bytes; i++)
            if (strncmp(&buffer[i], data->word, word_length) == 0)
                count++;

        if (ftell(file) != data->end)
            fseek(file, -word_length + 1, SEEK_CUR);
    }
    fclose(file);
    //printf("chunks read: %d\n", chunks);

    return (void*)(intptr_t)count;
}

void processFile(char* filename, char* word, int thread_count) {
    FILE* file = fopen(filename, "rb");
    int file_size = filelength(file);
    int segment_size = file_size / thread_count;
    int extra_bytes = strlen(word) - 1;

    pthread_t threads[thread_count];
    ThreadData thread_data[thread_count];

    //printf("Segment size is: %d with %d thread(s)\n", segment_size, thread_count);

    for (int t = 0; t < thread_count; t++) {
        // thread_data[t].file = file;
        thread_data[t].filename = filename;
        thread_data[t].word = word;
        thread_data[t].start = t * segment_size - (t > 0 ? extra_bytes : 0);
        thread_data[t].end = (t + 1) * segment_size;

        pthread_create(&threads[t], NULL, countWord, &thread_data[t]);
    }

    int total_count = 0;
    for (int t = 0; t < thread_count; t++) {
        void* count;
        pthread_join(threads[t], &count);
        total_count += (int)(intptr_t)count;
        //printf("%d was just added to total\n", (int)(intptr_t)count);
    }

    printf("%s - %d\n", filename, total_count);
    fclose(file);
}

struct timespec start, finish;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Incorrect number of arguments\n");
        exit(1);
    }

    int thread_count = atoi(argv[1]);
    char word[MAXWORDSIZE];

    printf("Enter a word: ");

	fgets(word, sizeof(word), stdin); 
        word[strcspn(word, "\n")] = 0;

    double elapsed;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 2; i < argc; i++) {
        pid_t pid = fork();
        if (pid < 0) 
            exit(1);
        if (pid == 0) {
            processFile(argv[i], word, thread_count);
            exit(0); 
        }
    }

    for (int i = 1; i < argc; i++)
        wait(NULL);

    clock_gettime(CLOCK_MONOTONIC, &finish);
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Execution time: %lf seconds\n", elapsed);

    fflush(stdin);

    return 0;
}
