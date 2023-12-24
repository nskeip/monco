#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRIES_N 100

char *entries[MAX_ENTRIES_N] = {NULL};
size_t entries_n = 0;

void print_help_command(char short_name, const char *const long_name,
                        const char *const description) {
  printf("%10c, %-10s    %s\n", short_name, long_name, description);
}

bool str_eq(const char *a, const char *b) {
  while (1) {
    if (*a != *b) {
      return false;
    }
    if (*a == '\0') {
      return true;
    }
    a++;
    b++;
  }
}

typedef enum {
  TOKEN_TYPE_STR,
  TOKEN_TYPE_OP_OR,
  TOKEN_TYPE_OP_AND,
  TOKEN_TYPE_PAR_OPEN,
  TOKEN_TYPE_PAR_CLOSE,
} TokenType;

typedef struct {
  TokenType type;
  char *str;
} Token;

typedef struct {
  size_t tokens_n;
  Token *tokens;
} TokenList;

TokenList *token_list_init(void) {
  TokenList *token_list = malloc(sizeof(TokenList));
  if (token_list == NULL) {
    return NULL;
  }
  token_list->tokens_n = 0;
  token_list->tokens = NULL;
  return token_list;
}

void token_list_destroy(TokenList *token_list) {
  for (size_t i = 0; i < token_list->tokens_n; i++) {
    if (token_list->tokens[i].type == TOKEN_TYPE_STR) {
      free(token_list->tokens[i].str);
      token_list->tokens[i].str = NULL;
    }
  }
  free(token_list->tokens);
  token_list->tokens = NULL;
  free(token_list);
}

TokenList *tokenize(const char *s) {
  // This treats whitespace after a word as a part of a token
  TokenList *result = token_list_init();
  if (result == NULL) {
    fprintf(stderr, "Failed to allocate memory for token list!\n");
    return NULL;
  }
  while (*s != '\0') {
    if (*s == ' ') {
      // dealing with the "leftmost" whitespaces, not belonging
      // to any token
      s++;
      continue;
    }

    const size_t resize_block_size = 10;
    if (result->tokens_n % resize_block_size == 0) {
      result->tokens =
          realloc(result->tokens,
                  (result->tokens_n + resize_block_size) * sizeof(Token));
    }

    switch (*s) {
    case '|':
      result->tokens[result->tokens_n++] =
          (Token){.type = TOKEN_TYPE_OP_OR, .str = NULL};
      s++;
      break;
    case '&':
      result->tokens[result->tokens_n++] =
          (Token){.type = TOKEN_TYPE_OP_AND, .str = NULL};
      s++;
      break;
    case '(':
      result->tokens[result->tokens_n++] =
          (Token){.type = TOKEN_TYPE_PAR_OPEN, .str = NULL};
      s++;
      break;
    case ')':
      result->tokens[result->tokens_n++] =
          (Token){.type = TOKEN_TYPE_PAR_CLOSE, .str = NULL};
      s++;
      break;
    default: {
      size_t word_len_with_right_spaces = strcspn(s, "|&()");
      assert(word_len_with_right_spaces != 0);

      size_t word_len_trimmed = word_len_with_right_spaces;
      while (word_len_trimmed > 0 && s[word_len_trimmed - 1] == ' ') {
        word_len_trimmed--;
      }

      char *word_copy = strndup(s, word_len_trimmed);
      if (word_copy == NULL) {
        fprintf(stderr, "Failed to allocate memory for token! %s\n", s);
        token_list_destroy(result);
        return NULL;
      }
      result->tokens[result->tokens_n++] =
          (Token){.type = TOKEN_TYPE_STR, .str = word_copy};
      s += word_len_with_right_spaces;
    }
    }
  }
  return result;
}

int precedence(TokenType type) {
  switch (type) {
  case TOKEN_TYPE_OP_OR:
    return 1;
  case TOKEN_TYPE_OP_AND:
    return 2;
  default:
    return 0;
  }
}

Token token_list_peek(TokenList *token_list) {
  assert(token_list->tokens_n != 0);
  return token_list->tokens[token_list->tokens_n - 1];
}

void token_list_drop_last_element(TokenList *token_list) {
  assert(token_list->tokens_n != 0);
  token_list->tokens[--token_list->tokens_n] = (Token){0};
}

TokenList *make_postfix_notation(const TokenList *const token_list) {
  TokenList *output_queue = token_list_init();
  TokenList *op_stack = token_list_init();

  // allocating maximum possible sizes
  output_queue->tokens = calloc(token_list->tokens_n, sizeof(Token));
  op_stack->tokens = calloc(token_list->tokens_n, sizeof(Token));

  for (size_t i = 0; i < token_list->tokens_n; i++) {
    switch (token_list->tokens[i].type) {
    case TOKEN_TYPE_STR:
      output_queue->tokens[output_queue->tokens_n++] = token_list->tokens[i];
      break;
    case TOKEN_TYPE_OP_OR:
    case TOKEN_TYPE_OP_AND: {
      while (op_stack->tokens_n != 0) {
        Token op2 = token_list_peek(op_stack);
        if (op2.type == TOKEN_TYPE_PAR_OPEN) {
          break;
        }
        if (precedence(token_list->tokens[i].type) <= precedence(op2.type)) {
          // left-associativity check should be in a separate if
          // but both operators are left-associative (even more: they are
          // associative) - so we just put `<=` instead of `<` here.
          break;
        }
        // add to queue
        output_queue->tokens[output_queue->tokens_n++] = op2;
        token_list_drop_last_element(op_stack);
      }
      // o1 to stack
      op_stack->tokens[op_stack->tokens_n++] = token_list->tokens[i];
      break;
    }
    case TOKEN_TYPE_PAR_OPEN:
      // push to stack
      op_stack->tokens[op_stack->tokens_n++] = token_list->tokens[i];
      break;
    case TOKEN_TYPE_PAR_CLOSE: {
      while (op_stack->tokens_n != 0) {
        Token op_from_stack = token_list_peek(op_stack);
        if (op_from_stack.type == TOKEN_TYPE_PAR_OPEN) {
          break;
        }
        // from stack to queue
        output_queue->tokens[output_queue->tokens_n++] = op_from_stack;
        token_list_drop_last_element(op_stack);
      }
      if (op_stack->tokens_n == 0 ||
          token_list_peek(op_stack).type != TOKEN_TYPE_PAR_OPEN) {
        fprintf(stderr, "Mismatched parentheses (while processing tokens)!\n");
        goto clean_up_err;
      }
      token_list_drop_last_element(op_stack); // drop '('
      /* we don't need function-before-( -- so just skipping it */
      break;
    }
    }
  }

  for (size_t i = op_stack->tokens_n - 1; i >= 0; --i) {
    if (op_stack->tokens[i].type == TOKEN_TYPE_PAR_OPEN) {
      fprintf(stderr, "Mismatched parentheses (while building output)!\n");
      goto clean_up_err;
    }
    output_queue->tokens[output_queue->tokens_n++] = op_stack->tokens[i];
  }

  token_list_destroy(op_stack);
  return output_queue;

clean_up_err:
  token_list_destroy(output_queue);
  token_list_destroy(op_stack);
  return NULL;
}

void run_tests(void) {
  {
    TokenList *token_list = tokenize("Alice & (Bob |Charlie Chaplin)");
    assert(token_list->tokens_n == 7);

    assert(token_list->tokens[0].type == TOKEN_TYPE_STR);
    assert(str_eq(token_list->tokens[0].str, "Alice"));

    assert(token_list->tokens[1].type == TOKEN_TYPE_OP_AND);
    assert(token_list->tokens[1].str == NULL);

    assert(token_list->tokens[2].type == TOKEN_TYPE_PAR_OPEN);
    assert(token_list->tokens[2].str == NULL);

    assert(token_list->tokens[3].type == TOKEN_TYPE_STR);
    assert(str_eq(token_list->tokens[3].str, "Bob"));

    assert(token_list->tokens[4].type == TOKEN_TYPE_OP_OR);
    assert(token_list->tokens[4].str == NULL);

    assert(token_list->tokens[5].type == TOKEN_TYPE_STR);
    assert(str_eq(token_list->tokens[5].str, "Charlie Chaplin"));

    assert(token_list->tokens[6].type == TOKEN_TYPE_PAR_CLOSE);
    assert(token_list->tokens[6].str == NULL);

    token_list_destroy(token_list);
  }
  printf("\x1b[32m"); // green text
  printf("\u2713 ");  // Unicode check mark
  printf("\x1b[0m");  // Reset text color to default
  printf("All tests passed\n");
}

int process_user_input(const char *const input) {
  if (*input == '\0') {
    return 0;
  }
  if (str_eq(input, "help") || str_eq(input, "h")) {
    puts("Available commands:");
    print_help_command('a', "add", "Add an entry");
    print_help_command('d', "del", "Delete an entry");
    print_help_command('h', "help", "Read this help");
    print_help_command('l', "list", "List all entries");
    print_help_command('s', "search", "Search for an entry");
    print_help_command('q', "quit", "Quit the application");
  } else if (str_eq(input, "add") || str_eq(input, "a")) {
    if (entries_n == MAX_ENTRIES_N) {
      puts("Maximum number of entries reached! Will not add more.");
      return 0;
    }
    printf("Enter text: ");
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
  } else if (str_eq(input, "del") || str_eq(input, "d")) {
    if (entries_n == 0) {
      puts("No entries to delete!");
      return 0;
    }
    printf("Enter number: ");
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
  } else if (str_eq(input, "list") || str_eq(input, "l")) {
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
  } else if (str_eq(input, "search") || str_eq(input, "s")) {
    printf("Search: ");
    size_t pattern_initial_size = 256;
    char *pattern = malloc(pattern_initial_size);
    if (getline(&pattern, &pattern_initial_size, stdin) == -1) {
      free(pattern);
      fprintf(stderr, "Failed to read search pattern! Try again\n");
      return 0;
    }
    char *end = strchr(pattern, '\n');
    if (end != NULL) {
      *end = '\0';
    }
    for (size_t i = 0; i < entries_n; i++) {
      if (strstr(entries[i], pattern) != NULL) {
        printf("%zu) %s\n", i, entries[i]);
      }
    }
  } else if (str_eq(input, "quit") || str_eq(input, "q")) {
    return 1;
  } else {
    fprintf(stderr, "Unknown command: %s. Type 'help' for help.\n", input);
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    if (str_eq(argv[1], "--help") || str_eq(argv[1], "-h")) {
      puts("Usage:");
      print_help_command('h', "--help", "Display this help message");
      print_help_command('t', "--test", "Run tests");
      return EXIT_SUCCESS;
    }
    if (str_eq(argv[1], "--test") || str_eq(argv[1], "-t")) {
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
    if (process_user_input(input) != 0) {
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
