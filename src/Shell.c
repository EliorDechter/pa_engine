
/*
create_entity 'name' 
set_entity 'property' 'value' (properties: animation, texture...)
set_current_scene 'name'
add_entity_to_scene 'name'
load_texture 'name'
add_texture_to_scene 'name'
set_background 'name'



*/



typedef enum Command_token {
    command_token_create,
    command_token_set,
    command_token_load,
    command_token_name,
    command_token_number,
} Command_token;

void interpret_command(String_128 str) {
    //lex command
    
    
    
    
}

typedef enum Shell_command_type {
    shell_command_type_load
} Shell_command_type;

typedef enum Shell_load_command_type {
    shell_load_command_type_texture
} Shell_load_command_type;

typedef enum Shell_set_command_type {
    shell_set_command_type_entity
} Shell_set_command_type;

typedef struct Shell_command {
    Shell_command_type type;
    union {
        struct {
            Shell_load_command_type type;
            char path[standrad_name_length];
            char name[standrad_name_length];
        } load_args;
        struct {
            Shell_set_command_type type;
        } set_args;
    } args;
} Shell_command;

typedef struct Shell {
    
} Shell;

void run_shell_command(Shell_command command) {
    
}
