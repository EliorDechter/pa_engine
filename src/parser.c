#ifndef PA_PARSER
#define PA_PARSER

typedef enum Token_type {
    token_type_eof,
    token_type_equal,
    token_type_arrow,
    token_type_colon,
    token_type_semi_colon,
    token_type_left_brace,
    token_type_right_brace,
    token_type_float,
    token_type_int,
    token_type_dot,
    token_type_keyword,
    token_type_name,
    token_type_comma
} Token_type;

typedef struct Token_pos {
    const char *name;
    int line;
} Token_pos;

typedef struct Token {
    Token_type type;
    Token_pos pos;
    const char *start, *end;
    union {
        s32 int_val;
        float float_val;
        const char *name;
    };
} Token;

typedef struct Tokenizer {
    const char *line_start;
    const char *stream;
    Token token;
    
    bool error;
} Tokenizer;

Tokenizer g_tokenizer;

const char *token_type_names[] = {
    [token_type_eof] = "eof",
    [token_type_arrow] = "->",
    [token_type_colon] = ":",
    [token_type_left_brace] = "{",
    [token_type_right_brace] = "}",
    [token_type_equal] = "=",
    [token_type_colon] = ":",
    [token_type_semi_colon] = ";",
    [token_type_dot] = ".", 
    [token_type_comma] = ",",
};

void parser_error(const char *fmt, ...) {
    //builtin stuff
    va_list args;
    va_start(args, fmt);
    printf("%s (%d): error: ", g_tokenizer.token.pos.name, g_tokenizer.token.pos.line);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

#define fatal_parser_error(...) (parser_error(__VA_ARGS__), assert(0))

void scan_int() {
    int base = 10;
    const char *start_digits = g_tokenizer.stream;
    int val = 0;
    while (1) {
        if (!isdigit(*g_tokenizer.stream)) {
            break;
        }
        s32 digit = *g_tokenizer.stream - '0';
        val = val * base + digit;
        g_tokenizer.stream++;
    }
    
    g_tokenizer.token.type = token_type_int;
    g_tokenizer.token.int_val = val;
}

void scan_float() {
    const char *start = g_tokenizer.stream;
    while (*g_tokenizer.stream == '-') {
        ++g_tokenizer.stream;
    }
    while (isdigit(*g_tokenizer.stream)) {
        ++g_tokenizer.stream;
    }
    if (*g_tokenizer.stream == '.') {
        ++g_tokenizer.stream;
    }
    while (isdigit(*g_tokenizer.stream)) {
        ++g_tokenizer.stream;
    }
    if (*g_tokenizer.stream == 'f') {
        ++g_tokenizer.stream;
    }
    else if (tolower(*g_tokenizer.stream) == 'e') {
        ++g_tokenizer.stream;
        if (*g_tokenizer.stream == '+' || *g_tokenizer.stream == '-') {
            ++g_tokenizer.stream;
        }
        if (!isdigit(*g_tokenizer.stream)) {
            parser_error("Expected digit after float literal exponent, found '%c'.", *g_tokenizer.stream);
        }
        while (isdigit(*g_tokenizer.stream)) {
            ++g_tokenizer.stream;
        }
    }
    float val = strtod(start, null);
    //
    g_tokenizer.token.type = token_type_float;
    g_tokenizer.token.float_val = val;
}

const char *str_intern_range(const char *start, const char *end) {
    s32 len = end - start;
    char *str = (char *)allocate_perm(g_allocator, len + 1); //TODO: do I want a perm or frame allocator?
    memcpy(str, start, len);
    str[len] = 0;
    
    return str;
}

void get_next_token() {
    Token *token = &g_tokenizer.token;
    repeat:
    token->start = g_tokenizer.stream;
    
    s32 num_multiplier = 1;
    
    switch(*g_tokenizer.stream) {
        case ' ': case '\n': case '\r': case '\t': case '\v': {
            while (isspace(*g_tokenizer.stream)) {
                if (*g_tokenizer.stream++ == '\n') {
                    g_tokenizer.line_start = g_tokenizer.stream;
                    token->pos.line++;
                }
            }
            goto repeat;
        }
        case '{': {
            ++g_tokenizer.stream;
            g_tokenizer.token.type = token_type_left_brace;
            break;
        }
        case '}': {
            ++g_tokenizer.stream;
            g_tokenizer.token.type = token_type_right_brace;
            break;
        }
        case '=': {
            ++g_tokenizer.stream;
            g_tokenizer.token.type = token_type_equal;
            break;
        }
        case '\'': {
            //scan_char();
            break;
        }
        case '"': {
            //scan_str();
            break;
        }
        case '.': {
            if (isdigit(g_tokenizer.stream[1])) {
                scan_float();
            }
            else {
                token->type = token_type_dot;
                ++g_tokenizer.stream;
            }
            break;
        }
        case '-': {
            ++g_tokenizer.stream;
        }
        case '0' ... '9': {
            while (isdigit(*g_tokenizer.stream)) {
                ++g_tokenizer.stream;
            }
            char c = *g_tokenizer.stream;
            g_tokenizer.stream = g_tokenizer.token.start;
            if (c == '.' || tolower(c) == 'e') {
                scan_float();
            }
            else {
                scan_int();
            }
            break;
        }
        case ',': {
            token->type = token_type_comma;
            g_tokenizer.stream++;
            break;
        }
        case 'a' ... 'z':
        case 'A' ... 'Z': {
            while (isalnum(*g_tokenizer.stream) || *g_tokenizer.stream == '_') {
                g_tokenizer.stream++;
            }
            token->name = str_intern_range(token->start, g_tokenizer.stream);
            token->type = token_type_name;
            break;
        }
        case '\0': {
            token->type = token_type_eof;
            break;
        }
        default: {
            parser_error("Invalid '%c' token, skipping ", *g_tokenizer.stream);
            g_tokenizer.stream++;
            goto repeat;
        }
    }
    token->end = g_tokenizer.stream;
}

void init_stream(const char *stream) {
    g_tokenizer.stream = stream;
    g_tokenizer.line_start = stream;
    g_tokenizer.token.pos.name = "<string>";
    g_tokenizer.token.pos.line = 1;
    get_next_token();
}

bool is_token(Token_type type) {
    if (g_tokenizer.token.type == type)
        return true;
    return false;
}

bool match_token(Token_type type) {
    if (is_token(type)) {
        get_next_token();
        return true;
    }
    return false;
}

const char *get_token_type_name(Token_type type) {
    if (type < array_count(token_type_names)) {
        return token_type_names[type];
    }
    
    return "<unknown>";
}

const char *token_info() {
    Token token = g_tokenizer.token;
    if (token.type == token_type_name || token.type == token_type_keyword) {
        return token.name;
    }
    
    return get_token_type_name(token.type);
}

bool expect_token(Token_type type) {
    if (is_token(type)) {
        get_next_token();
        return true;
    }
    
    parser_error("Expected token '%s', got '%s'", get_token_type_name(type), token_info());
    return false;
}

bool is_token_eof() {
    return g_tokenizer.token.type == token_type_eof;
}

float *parse_vertex_buffer(const char *file_name) {
    const char *vertices = load_file_to_buffer(file_name);
    float *buffer = null;
    const char *name = null;
    init_stream(vertices);
    
    expect_token(token_type_name);
    name = g_tokenizer.token.name;
    expect_token(token_type_equal);
    expect_token(token_type_left_brace);
    while (!is_token_eof()) {
        if (match_token(token_type_int)) {
            Token token = g_tokenizer.token;
            arrput(buffer,token.int_val);
            if (match_token(token_type_right_brace)) {
                //printf("'%d'", g_tokenizer.token.int_val);
                break;
            }
            else {
                //printf("'%d', ", g_tokenizer.token.int_val);
                expect_token(token_type_comma);
            }
        }
        else if (match_token(token_type_float)) {
            Token token = g_tokenizer.token;
            arrput(buffer,token.float_val);
            if (match_token(token_type_right_brace)) {
                //printf("'%f'", token.float_val);
                break;
            }
            else {
                //printf("'%f', ", token.float_val);
                expect_token(token_type_comma);
            }
        }
        else {
            parser_error("");
        }
    }
    
    //printf("parsed successfuly \n");
    
    return buffer;
}
#endif 