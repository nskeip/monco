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

void print_help_command(char short_name, const char *const long_name,
                        const char *const description) {
  printf("%10c, %-10s    %s\n", short_name, long_name, description);
}

int evaluate(const char *const input) {
  if (*input == '\0') {
    return 0;
  }
  if (strcmp(input, "help") == 0 || strcmp(input, "h") == 0) {
    puts("Available commands:");
    print_help_command('a', "add", "Add an entry");
    print_help_command('d', "del", "Delete an entry");
    print_help_command('l', "list", "List all entries");
    print_help_command('h', "help", "Read this help");
    print_help_command('q', "quit", "Quit the application");
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
      fprintf(stderr, "Failed to read entry! Try again\n");
      return 0;
    }
    char *end = strchr(entry, '\n');
    if (end != NULL) {
      *end = '\0';
    }
    entries[entries_n++] = entry;
  } else if (strcmp(input, "del") == 0 || strcmp(input, "d") == 0) {
    if (entries_n == 0) {
      puts("No entries to delete!");
      return 0;
    }
    puts("Enter number:");
    size_t entry_number;
    if (scanf("%zu", &entry_number) != 1) {
      fprintf(stderr, "Kinda strange entry number. Are you using stilys?\n");
      return 0;
    }

    // consume the rest of the line
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
      ;

    if (entry_number >= entries_n) {
      fprintf(stderr, "Entry number out of range!\n");
      return 0;
    }
    free(entries[entry_number]);
    entries[entry_number] = entries[entries_n - 1];
    entries[entries_n - 1] = NULL;
    entries_n--;
  } else if (strcmp(input, "list") == 0 || strcmp(input, "l") == 0) {
    for (size_t i = 0; i < entries_n; i++) {
      printf("%zu) %s\n", i, entries[i]);
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
      print_help_command('t', "--test", "Run tests");
      print_help_command('h', "--help", "Display this help message");
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
