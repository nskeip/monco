#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRIES_N 100
#define MAX_ENTRY_LEN 100

char entries[MAX_ENTRIES_N][MAX_ENTRY_LEN] = {{'\0'}};

void run_tests() {
  assert(1 == 1);
  printf("\x1b[32m"); // green text
  printf("\u2713 ");  // Unicode check mark
  printf("\x1b[0m");  // Reset text color to default
  printf("All tests passed\n");
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
      puts("Usage:");
      printf("%10s, %-10s    %s\n", "-t", "--tests", "Run tests");
      printf("%10s, %-10s    %s\n", "-h", "--help",
             "Display this help message");
      return EXIT_SUCCESS;
    }
    if (strcmp(argv[1], "--test") == 0 || strcmp(argv[1], "-t") == 0) {
      run_tests();
      return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
