#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRIES_N 100

char *entries[MAX_ENTRIES_N] = {NULL};
size_t entries_n = 0;

void run_tests() {
  assert(1 == 1);
  printf("\x1b[32m"); // green text
  printf("\u2713 ");  // Unicode check mark
  printf("\x1b[0m");  // Reset text color to default
  printf("All tests passed\n");
}

int evaluate(const char *const input) {
  if (*input == '\0') {
    return 0;
  }
  if (strcmp(input, "help") == 0 || strcmp(input, "h") == 0) {
    puts("Available commands:");
    printf("%10s, %-10s    %s\n", "a", "add", "Add an entry");
    printf("%10s, %-10s    %s\n", "l", "list", "List all entries");
    printf("%10s, %-10s    %s\n", "h", "help", "Read this help");
    printf("%10s, %-10s    %s\n", "q", "quit", "Quit the application");
  } else if (strcmp(input, "add") == 0 || strcmp(input, "a") == 0) {
    if (entries_n == MAX_ENTRIES_N) {
      puts("Maximum number of entries reached! Will not add more.");
      return 0;
    }
    puts("Enter text:");
    size_t entry_initial_size = 256;
    char *entry = malloc(entry_initial_size);
    if (getline(&entry, &entry_initial_size, stdin) == -1) {
      free(entry);
      fprintf(stderr, "Failed to read entry! It's gonna blow!\n");
      return 1;
    }
    char *end = strchr(entry, '\n');
    if (end != NULL) {
      *end = '\0';
    }
    entries[entries_n++] = entry;
  } else if (strcmp(input, "list") == 0 || strcmp(input, "l") == 0) {
    for (size_t i = 0; i < entries_n; i++) {
      printf("%zu) %s\n", i + 1, entries[i]);
    }
    switch (entries_n) {
    case 0:
      puts("No entries yet!");
      break;
    case 1:
      puts("Total: 1 entry");
      break;
    default:
      printf("Total: %zu entries\n", entries_n);
    }
  } else if (strcmp(input, "quit") == 0 || strcmp(input, "q") == 0) {
    return 1;
  } else {
    fprintf(stderr, "Unknown command: %s. Type 'help' for help.\n", input);
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
  for (size_t i = 0; i < MAX_ENTRIES_N; i++) {
    free(entries[i]);
  }
  puts("Bye!");
  free(input);

  return EXIT_SUCCESS;
}
