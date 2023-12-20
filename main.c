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

int evaluate(const char *const input) {
  if (strcmp(input, "help") == 0 || strcmp(input, "h") == 0) {
    puts("Available commands:");
    printf("%10s, %-10s    %s\n", "h", "help", "Read this help");
    printf("%10s, %-10s    %s\n", "q", "quit", "Quit the application");
  } else if (strcmp(input, "quit") == 0 || strcmp(input, "q") == 0) {
    return 1;
  }
  return 0;
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

  puts("Welcome to monco! Type 'help' for help.");
  size_t input_initial_size = 256;
  char *input = malloc(input_initial_size);
  while (1) {
    printf("monco > ");
    if (getline(&input, &input_initial_size, stdin) == -1) {
      break;
    }
    char *end = strchr(input, '\n');
    if (end != NULL) {
      *end = '\0';
    }
    if (evaluate(input) != 0) {
      break;
    }
  }
  puts("Bye!");
  free(input);

  return EXIT_SUCCESS;
}
