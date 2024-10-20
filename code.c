#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "symbol_table.h"

YYSTYPE yylval;
TokenType token;
SymbolTable* symbol_table;

void init_parser() {
    symbol_table = create_symbol_table();
}

void cleanup_parser() {
    free_symbol_table(symbol_table);
}

void error(const char* message) {
    fprintf(stderr, "Error: %s\n", message);
    exit(1);
}

void eat(TokenType _token) {
    if (token == _token) {
        token = yylex();
    } else {
        error("Unexpected token");
    }
}

ASTNode* parse_expression();
ASTNode* parse_variable_declaration();

void free_ast(ASTNode* node);


ASTNode* create_function_node(char* return_type, char* name, ASTNode* body) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = NODE_FUNCTION;
    node->data.function.return_type = return_type;
    node->data.function.name = name;
    node->data.function.body = body;
    return node;
}

ASTNode* create_log_node(LogElement* elements) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = NODE_LOG;
    node->data.log.elements = elements;
    return node;
}

ASTNode* create_binary_expr_node(ASTNode* left, ASTNode* right, char operator) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = NODE_BINARY_EXPR;
    node->data.binary_expr.left = left;
    node->data.binary_expr.right = right;
    node->data.binary_expr.operator = operator;
    return node;
}

ASTNode* create_number_node(int value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = NODE_NUMBER;
    node->data.number.value = value;
    return node;
}

ASTNode* create_string_node(char* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = NODE_STRING;
    node->data.string.value = strdup(value);
    return node;
}

ASTNode* create_variable_node(char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = NODE_VARIABLE;
    node->data.variable.name = strdup(name);
    return node;
}

ASTNode* parse_factor() {
    if (token == NUMBER) {
        ASTNode* node = create_number_node(yylval.number);
        eat(NUMBER);
        return node;
    } else if (token == STRING) {
        ASTNode* node = create_string_node(yylval.string);
        eat(STRING);
        return node;
    } else if (token == IDENTIFIER) {
        ASTNode* node = create_variable_node(yylval.string);
        eat(IDENTIFIER);
        return node;
    } else if (token == LPAREN) {
        eat(LPAREN);
        ASTNode* node = parse_expression();
        eat(RPAREN);
        return node;
    }
    error("Unexpected token");
    return NULL;
}

ASTNode* parse_term() {
    ASTNode* node = parse_factor();
    while (token == MULTIPLY || token == DIVIDE) {
        char op = (token == MULTIPLY) ? '*' : '/';
        eat(token);
        node = create_binary_expr_node(node, parse_factor(), op);
    }
    return node;
}

ASTNode* parse_expression() {
    ASTNode* node = parse_term();
    while (token == PLUS || token == MINUS) {
        char op = (token == PLUS) ? '*' : '-';
        eat(token);
        node = create_binary_expr_node(node, parse_term(), op);
    }
    return node;
}

ASTNode* parse_variable_declaration() {
    char* type_name = strdup(yytext);
    eat(IDENTIFIER);

    char* var_name = strdup(yytext);
    eat(IDENTIFIER);

    DataType type;
    if (strcmp(type_name, "int") == 0) {
        type = TYPE_INT;
    } else if (strcmp(type_name, "string") == 0) {
        type = TYPE_STRING;
    } else {
        error("Unknown type");
        exit(1);
    }

    if (!add_symbol(symbol_table, var_name, type)) {
        error("Variable already declared");
        exit(1);
    }

    free(type_name);
    free(var_name);

    eat(SEMICOLON);
    // return create_variable_node(var_name);
    return NULL;
}

ASTNode* parse_log() {
    // eat(LOG);
    // eat(LPAREN);
    // char* message = yylval.string;
    // printf("Message: %s\n", message);
    // eat(STRING);
    // eat(RPAREN);
    // eat(SEMICOLON);
    // return create_log_node(message);
    eat(LOG);
    eat(LPAREN);

    LogElement* head = NULL;
    LogElement* current = NULL;

    while (token != RPAREN) {
        LogElement* element = malloc(sizeof(LogElement));

        if (token == STRING) {
            element->type = NODE_STRING;
            element->value.string = strdup(yylval.string);
            eat(STRING);
        } else if (token == PLUS) {
            if (current->type == NODE_STRING) {
                element->type = NODE_STRING;
                element->value.string = strdup(" ");
                eat(PLUS);
            }
        } else {
            ASTNode* expr = parse_expression();
            DataType type = get_expression_type(expr, symbol_table);

            if (type == TYPE_INT) {
                element->type = NODE_NUMBER;
                element->value.number = expr->data.number.value;
            } else if (type == TYPE_STRING) {
                element->type = NODE_STRING;
                element->value.string = strdup(expr->data.string.value);
            } else {
                error("Invalid type in log statement");
            }

            free_ast(expr);
        }

        if (head == NULL) {
            head = element;
            current = element;
        } else {
            current->next = element;
            current = element;
        }

        // if (token == PLUS) {
        //     eat(PLUS);
        // } else {
        //     break;
        // }
    }

    eat(RPAREN);
    eat(SEMICOLON);

    return create_log_node(head);
}

ASTNode* parse_statement() {
    switch (token) {
        case LOG:
            return parse_log();
        case NUMBER:
        case LPAREN:
            {
                ASTNode* expr = parse_expression();
                eat(SEMICOLON);
                return expr;
            }
        case INT:
        case STRING:
            return parse_variable_declaration();
        default:
            {
                error("Unexpected statement");
                return NULL;
            }
    }
}

ASTNode* parse_function() {
    char* return_type = strdup(yylval.string);  // We know it's "int" at this point
    if (!return_type) {
        error("Memory allocation error");
    }
    if (token == INT) {
        eat(INT);
    } else if (token == VOID) {
        eat(VOID);
    }
    
    printf("Token: %d\n", token);
    printf("Main: %d\n", MAIN);
    if (token != MAIN) {
        error("Expected MAIN");
    }
    // Get the function name
    char* name = strdup(yylval.string);
    printf("Function name: %s\n", name);
    eat(MAIN);
    eat(LPAREN);
    eat(RPAREN);
    eat(LBRACE);
    ASTNode* body = NULL;
    ASTNode* last = NULL;
    while (token != RBRACE) {
        ASTNode* statement = parse_statement();
        if (body == NULL) {
            body = statement;
            last = statement;
        } else {
            last->next = statement;
            last = statement;
        }
    }
    eat(RBRACE);
    return create_function_node(return_type, name, body);
}

ASTNode* parse() {
    return parse_function();
}

void generate_log_statement(FILE* output, LogElement* elements, int indent_level) {
    char indent[256] = {0};
    for (int i=0; i < indent_level; i++) {
        strcat(indent, "    ");
    }

    fprintf(output, "%sprintf(\"", indent);

    LogElement* current = elements;
    int arg_count = 0;
    while (current != NULL) {
        switch (current->type) {
            case NODE_STRING:
                fprintf(output, "%s", current->value.string);
                break;
            case NODE_NUMBER:
                fprintf(output, "%%d");
                arg_count++;
                break;
            case NODE_VARIABLE: {
                Symbol* symbol = lookup_symbol(symbol_table, current->value.string);
                if (symbol == NULL) {
                    error("Undefined variable in log statement");
                } else if (symbol->type == TYPE_INT) {
                    fprintf(output, "%%d");
                } else if (symbol->type == TYPE_STRING) {
                    fprintf(output, "%%s");
                } else {
                    error("Invalid type in log statement");
                }
                arg_count++;
                break;
            }
            default:
                error("Unknown node type in log statement");
        }
        current = current->next;
    }

    fprintf(output, "\\n\"");

    current = elements;
    while (current != NULL) {
        if (current->type == NODE_NUMBER) {
            fprintf(output, ", %d", current->value.number);
        } else if (current->type == NODE_VARIABLE) {
            fprintf(output, ", %s", current->value.string);
        }
        current = current->next;
    }

    fprintf(output, ");\n");
}

void generate(FILE* output, ASTNode* node, int indent_level) {
    char indent[256] = {0};
    for (int i=0; i < indent_level; i++) {
        strcat(indent, "    ");
    }

    switch (node->type) {
        case NODE_FUNCTION:
            fprintf(output, "%s %s() {\n", node->data.function.return_type, node->data.function.name);
            ASTNode* statement = node->data.function.body;
            while (statement != NULL) {
                generate(output, statement, indent_level + 1);
                statement = statement->next;
            }
            if (strcmp(node->data.function.return_type, "void") == 0) {
                fprintf(output, "%s    return;\n", indent);
            } else {
                fprintf(output, "%s    return 0;\n", indent);
            }
            fprintf(output, "}\n");
            break;
        case NODE_LOG:
            generate_log_statement(output, node->data.log.elements, indent_level);
            break;
        case NODE_BINARY_EXPR:
            fprintf(output, "%s", indent);
            generate(output, node->data.binary_expr.left, 0);
            fprintf(output, " %c ", node->data.binary_expr.operator);
            generate(output, node->data.binary_expr.right, 0);
            fprintf(output, ";\n");
            break;
        case NODE_NUMBER:
            fprintf(output, "%d", node->data.number.value);
            break;
        case NODE_STRING:
            fprintf(output, "\"%s\"", node->data.string.value);
            break;
        case NODE_VARIABLE:
            fprintf(output, "%s", node->data.variable.name);
            break;
        default:
            fprintf(stderr, "Unknown node type in AST\n");
            exit(1);
    }
}

void generate_code(FILE* output, ASTNode* node) {
    fprintf(output, "#include <stdio.h>\n\n");
    generate(output, node, 0);
}

void free_log_elements(LogElement* elements) {
    LogElement* current = elements;
    while (current != NULL) {
        LogElement* next = current->next;
        if (current->type == NODE_STRING) {
            free(current->value.string);
        }
        free(current);
        current = next;
    }
}

void free_ast(ASTNode* node) {
    if (node) {
        switch (node->type) {
            case NODE_FUNCTION:
                free(node->data.function.return_type);
                free(node->data.function.name);
                free_ast(node->data.function.body);
                break;
            case NODE_LOG:
                free_log_elements(node->data.log.elements);
                break;
            case NODE_BINARY_EXPR:
                free_ast(node->data.binary_expr.left);
                free_ast(node->data.binary_expr.right);
                break;
            case NODE_STRING:
                free(node->data.string.value);
                break;
            case NODE_VARIABLE:
                free(node->data.variable.name);
                break;
            case NODE_NUMBER:
                break;
        }
        if (node->next) free(node->next);
        free(node);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.w output.c\n", argv[0]);
        return 1;
    }

    FILE* input = fopen(argv[1], "r");
    if (!input) {
        perror("Error opening input file.");
        return 1;
    }

    FILE* output = fopen(argv[2], "w");
    if (!output) {
        perror("Error opening output file.");
        fclose(input);
        return 1;
    }

    yyin = input;
    token = yylex();
    printf("Token: %d\n", token);
    ASTNode* ast = parse();
    generate_code(output, ast);

    free_ast(ast);
    fclose(input);
    fclose(output);

    return 0;
}