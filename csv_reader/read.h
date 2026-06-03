#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CHUNK 64
typedef struct {
    char **headers;
    char ***values;
    int count;
} CsvData;

char* readLine(FILE *fp) {
    size_t lineSize = INITIAL_CHUNK;
    size_t curPosition = 0;
    char *buffer = malloc(lineSize);
    
    if (!buffer) return NULL;
    
    // We use a while loop to keep reading until we hit a newline or EOF
    while (fgets(buffer + curPosition, (int)(lineSize - curPosition), fp)) {
        size_t chunkLength = strlen(buffer + curPosition);
        curPosition += chunkLength;
        
        // Check if the line is complete (ends in newline)
        if (curPosition > 0 && buffer[curPosition - 1] == '\n') {
            return buffer;
        }
        
        // If we're here, the buffer was too small. Grow it!
        lineSize *= 2;
        char* temp = realloc(buffer, lineSize);
        if (!temp) {
            free(buffer);
            return NULL;
        }
        buffer = temp;
    }
    
    // Handle end-of-file: if we read something but didn't hit a newline
    if (curPosition > 0) {
        return buffer;
    }
    
    free(buffer);
    return NULL;
}

char **readFileToArray(char *fileName, size_t *outLineCount)
{
    printf("reading from file name: %s\n", fileName);
    FILE *fp = fopen(fileName, "r");
    if(!fp) {
        printf("error opening file");
        return NULL;
    }
    
    char* line;
    size_t count = 0;
    size_t capacity = 4;
    
    char **lines = malloc(capacity * sizeof(char *));
    if (!lines) {
        fclose(fp);
        return NULL;
    }
    
    while ((line = readLine(fp)) != NULL) {
        printf("line: %s\n", line);
        
        if(count >= capacity) {
            capacity *= 2;
            char **temp = realloc(lines, capacity * sizeof(char *));
            if (!temp) {
                // Clean up what we've allocated so far if realloc fails
                for (size_t i = 0; i < count; i++) free(lines[i]);
                free(lines);
                fclose(fp);
                return NULL;
            }
            lines = temp;
        }
        
        lines[count] = line;
        count++;
    }
    
    
    fclose(fp);
    *outLineCount = count;
    return lines;
}

int processLine(CsvData *csvData, char** myLines, int isHeader, int iter)
{
    int header_count = 0;
    int capacity = 16;
    char **items = malloc(sizeof(char *) * capacity);
    if (!items) return 0;
    header_count = 0;
    char* token = strtok(myLines[iter], ",");
    while(token != NULL) {
        if(header_count >= capacity) {
            printf("reallocate in processLine!\n");
            capacity *= 2; // Double the capacity dynamically
            char **temp = realloc(items, sizeof(char *) * capacity);
            if (!temp) {
                // Clean up what we allocated so far on failure
                for (int j = 0; j < header_count; j++) free(items[j]);
                free(items);
                return 0;
            }
            items = temp;
        }
        
        // Trim any trailing newline off the token if it's the last header
        size_t len = strlen(token);
        if (len > 0 && token[len - 1] == '\n') {
            token[len - 1] = '\0';
        }
        
        items[header_count] = strdup(token);
        header_count++;
        
        token = strtok(NULL, ",");
    }

    if(isHeader == 1) {
        csvData->headers = items;
        csvData->count = header_count;
    }
    else {
        csvData->values[iter-1] = items;
    }
    return header_count;
}

int readCsvFile(char *fileName)
{      
    size_t lineCount = 0;
    char **myLines = readFileToArray(fileName, &lineCount);
    
    if (!myLines) return 1;
    
    // Successfully populated! Let's print them out
    printf("--- Printing Stored Lines ---\n");
    CsvData csvData;
    csvData.headers = NULL;
    csvData.values = malloc(sizeof(char **) * 4);
    int header_count = 0;
    if(!csvData.values) {
        return 1;
    }
    
    for (size_t i = 0; i < lineCount; i++) {
        if(i == 0) {
            header_count = processLine(&csvData, myLines, 1, i);
        }
        else {
            header_count = processLine(&csvData, myLines, 0, i);
        }
    }
    
    // clean up my memory
    for (size_t i = 0; i < lineCount; i++) {
        free(myLines[i]);
    }
    free(myLines);
    
    
    for(int i=0; i < lineCount - 1; i++) { // Minus 1 to account for the header row
        for(int l=0; l < header_count; l++) {
            printf("item(%i): %s: %s\n", i, csvData.headers[l], csvData.values[i][l]);
        }
    }
    
    return 0;
}