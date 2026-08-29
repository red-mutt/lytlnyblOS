#include "user/programs/shell.h"

#include "user/libc/syscalls.h"
#include "user/libc/output.h"
#include "user/libc/chars.h"
#include "user/libc/memory.h"
#include "user/libc/string.h"


void start(void) {
  printf("hello this is output from the shell\n");
  
  //REPL
  for (;;) {
    //Read 
    char input[50];
    read(0, input, 50); 
    input[strlen(input) - 1] = '\0';

    //Eval
    size_t word_count;
    char** words = tokenize_line(input, &word_count);
    printf("we typed: %s, %d, %d, %s\n", input, strlen(input), word_count, words[0]);

    //Print
    if (!execute_command(words, word_count)) {
      printf("command failed\n");
    }

    //CLEANUP
    memset(input, 0, strlen(input));
    word_count = 0;
    free(words);


    
  }
}

bool execute_command(char** words, size_t word_count) {
  if (word_count > 2 || word_count < 2) {
    return false;
  }

  if (strcmp(words[0], "ls") == 0) {
    fs_ops(FS_LS, words[1]);
  }
  if (strcmp(words[0], "mkdir") == 0) {
    fs_ops(FS_MKDIR, words[1]);
  }
  if (strcmp(words[0], "touch") == 0) {
    fs_ops(FS_TOUCH, words[1]);
  }
  if (strcmp(words[0], "rm") == 0) {
    fs_ops(FS_RM, words[1]);
  }

  return true;
}


char** tokenize_line(const char* line, size_t *count_out) {
  const char* p = line;
  char **words;
  size_t words_i = 0;
  size_t capacity = 8;

  // limit of words in a line is 8
  words = calloc(capacity + 1, sizeof(char *));

  while (*p != '\0') {
    (*count_out)++;
    const char* start;

    while (isspace((unsigned char) *p)) {
      p++; 
    }

    if (*p == '\0') {
      break;
    }

    start = p;

    while (*p != '\0' && !isspace((unsigned char) *p)) {
      p++;
    }
    size_t len = p - start;
    char *word = malloc(len + 1);
    if (word == NULL) {
      return NULL;
    }

    memcpy(word, start, len);
    word[len] = '\0';
    if (word == NULL) {
      return NULL;
    }
    words[words_i++] = word;
  }
  return words;
}


