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
  TOKEN_TYPE_OP_NOT,
  TOKEN_TYPE_PAR_OPEN,
  TOKEN_TYPE_PAR_CLOSE,
  TOKEN_TYPE_TRUE,
  TOKEN_TYPE_FALSE,
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

void token_list_destroy_shallow(TokenList *token_list) {
  if (token_list == NULL) {
    return;
  }
  free(token_list->tokens);
  token_list->tokens = NULL;
  free(token_list);
}

void token_list_destroy_deep(TokenList *token_list) {
  if (token_list == NULL) {
    return;
  }
  for (size_t i = 0; i < token_list->tokens_n; i++) {
    if (token_list->tokens[i].type == TOKEN_TYPE_STR) {
      free(token_list->tokens[i].str);
      token_list->tokens[i].str = NULL;
    }
  }
  token_list_destroy_shallow(token_list);
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
    case '!':
      result->tokens[result->tokens_n++] =
          (Token){.type = TOKEN_TYPE_OP_NOT, .str = NULL};
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
      size_t token_str_len_with_right_spaces = strcspn(s, "|&()!");
      assert(token_str_len_with_right_spaces != 0);

      int token_str_len_trimmed = token_str_len_with_right_spaces;
      while (token_str_len_trimmed > 0 && s[token_str_len_trimmed - 1] == ' ') {
        token_str_len_trimmed--;
      }

      char *token_str = NULL;
      if (token_str_len_trimmed != 0) {
        token_str = strndup(s, token_str_len_trimmed);
        if (token_str == NULL) {
          fprintf(stderr, "Failed to allocate memory for token! %s\n", s);
          token_list_destroy_deep(result);
          return NULL;
        }
      }
      result->tokens[result->tokens_n++] =
          (Token){.type = TOKEN_TYPE_STR, .str = token_str};
      s += token_str_len_with_right_spaces;
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
  case TOKEN_TYPE_OP_NOT:
    return 3;
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
  memset(&token_list->tokens[--token_list->tokens_n], 0, sizeof(Token));
}

Token token_list_pop(TokenList *token_list) {
  Token result = token_list_peek(token_list);
  token_list_drop_last_element(token_list);
  return result;
}

TokenList *to_postfix_notation(const TokenList *const token_list) {
  TokenList *output_queue = token_list_init();
  if (output_queue == NULL) {
    fprintf(stderr, "Failed to allocate memory for output queue!\n");
    return NULL;
  }
  TokenList *op_stack = token_list_init();
  if (op_stack == NULL) {
    fprintf(stderr, "Failed to allocate memory for op stack!\n");
    goto clean_up_err;
  }

  // allocating maximum possible sizes
  output_queue->tokens = calloc(token_list->tokens_n, sizeof(Token));
  if (output_queue->tokens == NULL) {
    fprintf(stderr, "Failed to allocate memory for output queue tokens!\n");
    goto clean_up_err;
  }
  op_stack->tokens = calloc(token_list->tokens_n, sizeof(Token));
  if (op_stack->tokens == NULL) {
    fprintf(stderr, "Failed to allocate memory for op stack tokens!\n");
    goto clean_up_err;
  }

  for (size_t i = 0; i < token_list->tokens_n; i++) {
    Token current_tok = token_list->tokens[i];
    switch (current_tok.type) {
    case TOKEN_TYPE_STR:
      output_queue->tokens[output_queue->tokens_n++] = current_tok;
      break;
    case TOKEN_TYPE_OP_OR:
    case TOKEN_TYPE_OP_NOT:
    case TOKEN_TYPE_OP_AND: {
      while (op_stack->tokens_n != 0) {
        Token stack_op = token_list_peek(op_stack);
        if (stack_op.type == TOKEN_TYPE_PAR_OPEN) {
          break;
        }

        int current_tok_prec = precedence(current_tok.type);
        int stack_op_prec = precedence(stack_op.type);

        if (!(stack_op_prec > current_tok_prec ||
              (stack_op_prec == current_tok_prec &&
               current_tok.type != TOKEN_TYPE_OP_NOT))) {
          break;
        }
        // add to queue
        output_queue->tokens[output_queue->tokens_n++] = stack_op;
        token_list_drop_last_element(op_stack);
      }
      // o1 to stack
      op_stack->tokens[op_stack->tokens_n++] = current_tok;
      break;
    }
    case TOKEN_TYPE_PAR_OPEN:
      // push to stack
      op_stack->tokens[op_stack->tokens_n++] = current_tok;
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
    case TOKEN_TYPE_TRUE:
    case TOKEN_TYPE_FALSE:
      break;
    }
  }

  // going backwards, should know negative numbers :)
  for (int i = op_stack->tokens_n - 1; i >= 0; --i) {
    if (op_stack->tokens[i].type == TOKEN_TYPE_PAR_OPEN) {
      fprintf(stderr, "Mismatched parentheses (while building output)!\n");
      goto clean_up_err;
    }
    output_queue->tokens[output_queue->tokens_n++] = op_stack->tokens[i];
  }

  token_list_destroy_shallow(op_stack);
  return output_queue;

clean_up_err:
  token_list_destroy_shallow(output_queue);
  token_list_destroy_shallow(op_stack);
  return NULL;
}

bool eval_postfixed_tokens_as_predicate(const TokenList *const pf_list,
                                        const char *str) {
  if (pf_list == NULL) {
    fprintf(stderr, "NULL postfix list!\n");
    return false;
  }
  if (pf_list->tokens_n == 1) {
    if (pf_list->tokens[0].type != TOKEN_TYPE_STR) {
      fprintf(stderr, "Not a valid search pattern\n");
      return false;
    }
    return strcasestr(str, pf_list->tokens[0].str) != NULL;
  }

  TokenList *stack = token_list_init();
  if (stack == NULL) {
    fprintf(stderr, "Failed to allocate memory for stack!\n");
    goto cleanup;
  }

  // allocating maximum possible size
  stack->tokens = calloc(pf_list->tokens_n, sizeof(Token));
  if (stack->tokens == NULL) {
    fprintf(stderr, "Failed to allocate memory for stack tokens!\n");
    goto cleanup;
  }

  for (size_t i = 0; i < pf_list->tokens_n; i++) {
    Token current_tok = pf_list->tokens[i];
    switch (current_tok.type) {
    case TOKEN_TYPE_STR:
      stack->tokens[stack->tokens_n++] = current_tok;
      break;
    case TOKEN_TYPE_OP_NOT: {
      Token stack_token = token_list_pop(stack);
      if (stack_token.type == TOKEN_TYPE_STR) {
        bool str_of_token_found = strcasestr(str, stack_token.str) != NULL;
        stack->tokens[stack->tokens_n++] = (Token){
            .type = (str_of_token_found) ? TOKEN_TYPE_FALSE : TOKEN_TYPE_TRUE,
            .str = NULL};
      } else if (stack_token.type == TOKEN_TYPE_TRUE) {
        stack->tokens[stack->tokens_n++] =
            (Token){.type = TOKEN_TYPE_FALSE, .str = NULL};
      } else if (stack_token.type == TOKEN_TYPE_FALSE) {
        stack->tokens[stack->tokens_n++] =
            (Token){.type = TOKEN_TYPE_TRUE, .str = NULL};
      } else {
        fprintf(stderr, "Unexpected token type: %d\n", stack_token.type);
        assert(false);
      }
      break;
    }
    case TOKEN_TYPE_OP_OR:
    case TOKEN_TYPE_OP_AND: {
      Token op2 = token_list_pop(stack);
      Token op1 = token_list_pop(stack);

      bool op1_result;
      if (op1.type == TOKEN_TYPE_STR) {
        op1_result = strcasestr(str, op1.str) != NULL;
      } else {
        assert(op1.type == TOKEN_TYPE_TRUE || op1.type == TOKEN_TYPE_FALSE);
        op1_result = op1.type == TOKEN_TYPE_TRUE;
      }

      bool op2_result;
      if (op2.type == TOKEN_TYPE_STR) {
        op2_result = strcasestr(str, op2.str) != NULL;
      } else {
        assert(op2.type == TOKEN_TYPE_TRUE || op2.type == TOKEN_TYPE_FALSE);
        op2_result = op2.type == TOKEN_TYPE_TRUE;
      }

      bool new_value;
      if (current_tok.type == TOKEN_TYPE_OP_OR) {
        new_value = op1_result || op2_result;
      } else {
        new_value = op1_result && op2_result;
      }
      stack->tokens[stack->tokens_n++] =
          (Token){.type = (new_value) ? TOKEN_TYPE_TRUE : TOKEN_TYPE_FALSE,
                  .str = NULL};
      break;
    }
    default:
      printf("Unknown token type: %d\n", current_tok.type);
      assert(false);
    }
  }

  assert(stack->tokens_n == 1);
  bool result = stack->tokens[0].type == TOKEN_TYPE_TRUE;
  token_list_destroy_shallow(stack);
  return result;

cleanup:
  token_list_destroy_shallow(stack);
  return false;
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

    TokenList *pf_list = to_postfix_notation(token_list);

    assert(!eval_postfixed_tokens_as_predicate(pf_list, "Alice"));
    assert(!eval_postfixed_tokens_as_predicate(pf_list, "Bob"));
    assert(eval_postfixed_tokens_as_predicate(pf_list, "Alice and Bob"));

    assert(!eval_postfixed_tokens_as_predicate(pf_list, "Alice and Charlie"));
    assert(eval_postfixed_tokens_as_predicate(pf_list,
                                              "Alice and Charlie Chaplin"));

    token_list_destroy_shallow(pf_list);
    token_list_destroy_deep(token_list);
  }
  {
    TokenList *token_list = tokenize("alice");
    TokenList *pf_list = to_postfix_notation(token_list);
    assert(pf_list != NULL);
    assert(pf_list->tokens_n == 1);
    assert(pf_list->tokens[0].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[0].str, "alice"));
    assert(eval_postfixed_tokens_as_predicate(pf_list, "Alice in Wonderland"));
    token_list_destroy_shallow(pf_list);
    token_list_destroy_deep(token_list);
  }
  {
    // A + B * C + D -> A B C * + D +
    TokenList *token_list = tokenize("Alice | Bob & Charlie | Dan");
    assert(token_list->tokens_n == 7);

    TokenList *pf_list = to_postfix_notation(token_list);
    assert(pf_list != NULL);
    assert(pf_list->tokens_n == 7);

    assert(pf_list->tokens[0].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[0].str, "Alice"));

    assert(pf_list->tokens[1].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[1].str, "Bob"));

    assert(pf_list->tokens[2].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[2].str, "Charlie"));

    assert(pf_list->tokens[3].type == TOKEN_TYPE_OP_AND);

    assert(pf_list->tokens[4].type == TOKEN_TYPE_OP_OR);

    assert(pf_list->tokens[5].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[5].str, "Dan"));

    assert(pf_list->tokens[6].type == TOKEN_TYPE_OP_OR);

    assert(eval_postfixed_tokens_as_predicate(pf_list, "Alice"));
    assert(eval_postfixed_tokens_as_predicate(pf_list, "Dan"));
    assert(eval_postfixed_tokens_as_predicate(pf_list, "Alice and Dan"));

    assert(!eval_postfixed_tokens_as_predicate(pf_list, "Bob"));
    assert(!eval_postfixed_tokens_as_predicate(pf_list, "Charlie"));
    assert(eval_postfixed_tokens_as_predicate(pf_list, "Bob and Charlie"));

    token_list_destroy_shallow(pf_list);
    token_list_destroy_deep(token_list);
  }
  {
    // A + B * (C + D) -> A B C D + * +
    TokenList *token_list = tokenize("Alice | Bob & (Charlie | Dan)");
    assert(token_list->tokens_n == 9);

    TokenList *pf_list = to_postfix_notation(token_list);
    assert(pf_list != NULL);
    assert(pf_list->tokens_n == 7);

    assert(pf_list->tokens[0].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[0].str, "Alice"));

    assert(pf_list->tokens[1].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[1].str, "Bob"));

    assert(pf_list->tokens[2].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[2].str, "Charlie"));

    assert(pf_list->tokens[3].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[3].str, "Dan"));

    assert(pf_list->tokens[4].type == TOKEN_TYPE_OP_OR);
    assert(pf_list->tokens[5].type == TOKEN_TYPE_OP_AND);
    assert(pf_list->tokens[6].type == TOKEN_TYPE_OP_OR);

    token_list_destroy_shallow(pf_list);
    token_list_destroy_deep(token_list);
  }
  {
    TokenList *token_list = tokenize("!Alice | Bob");
    assert(token_list->tokens_n == 4);

    TokenList *pf_list = to_postfix_notation(token_list);
    assert(pf_list != NULL);
    assert(pf_list->tokens_n == 4);

    assert(pf_list->tokens[0].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[0].str, "Alice"));

    assert(pf_list->tokens[1].type == TOKEN_TYPE_OP_NOT);

    assert(pf_list->tokens[2].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[2].str, "Bob"));

    assert(pf_list->tokens[3].type == TOKEN_TYPE_OP_OR);

    assert(eval_postfixed_tokens_as_predicate(pf_list, "Bob")); // Bob is Bob
    assert(eval_postfixed_tokens_as_predicate(
        pf_list, "Charlie")); // Charlie is not Alice
    assert(!eval_postfixed_tokens_as_predicate(pf_list, "Alice"));

    token_list_destroy_shallow(pf_list);
    token_list_destroy_deep(token_list);
  }
  {
    TokenList *token_list = tokenize("!Alice | !!Bob");
    assert(token_list->tokens_n == 6);

    TokenList *pf_list = to_postfix_notation(token_list);
    assert(pf_list != NULL);
    assert(pf_list->tokens_n == 6);

    assert(pf_list->tokens[0].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[0].str, "Alice"));

    assert(pf_list->tokens[1].type == TOKEN_TYPE_OP_NOT);

    assert(pf_list->tokens[2].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[2].str, "Bob"));

    assert(pf_list->tokens[3].type == TOKEN_TYPE_OP_NOT);
    assert(pf_list->tokens[4].type == TOKEN_TYPE_OP_NOT);

    assert(pf_list->tokens[5].type == TOKEN_TYPE_OP_OR);

    assert(eval_postfixed_tokens_as_predicate(pf_list, "Bob")); // Bob is Bob
    assert(eval_postfixed_tokens_as_predicate(
        pf_list, "Charlie")); // Charlie is not Alice
    assert(!eval_postfixed_tokens_as_predicate(pf_list, "Alice"));

    token_list_destroy_shallow(pf_list);
    token_list_destroy_deep(token_list);
  }
  {
    TokenList *token_list = tokenize("!Alice & (Charlie | Dan)");
    assert(token_list->tokens_n == 8);

    TokenList *pf_list = to_postfix_notation(token_list);
    assert(pf_list != NULL);
    assert(pf_list->tokens_n == 6);

    assert(pf_list->tokens[0].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[0].str, "Alice"));

    assert(pf_list->tokens[1].type == TOKEN_TYPE_OP_NOT);

    assert(pf_list->tokens[2].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[2].str, "Charlie"));

    assert(pf_list->tokens[3].type == TOKEN_TYPE_STR);
    assert(str_eq(pf_list->tokens[3].str, "Dan"));

    assert(pf_list->tokens[4].type == TOKEN_TYPE_OP_OR);
    assert(pf_list->tokens[5].type == TOKEN_TYPE_OP_AND);

    assert(!eval_postfixed_tokens_as_predicate(pf_list, "Alice"));

    token_list_destroy_shallow(pf_list);
    token_list_destroy_deep(token_list);
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

    TokenList *token_list = tokenize(pattern);
    TokenList *pf_list = to_postfix_notation(token_list);
    for (size_t i = 0; i < entries_n; i++) {
      if (eval_postfixed_tokens_as_predicate(pf_list, entries[i])) {
        printf("%zu) %s\n", i, entries[i]);
      }
    }
    token_list_destroy_shallow(pf_list);
    token_list_destroy_deep(token_list);
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
