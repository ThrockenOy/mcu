#include "protocol-task.h"
#include <stdio.h>
#include <string.h>

static api_t* api = NULL;
static int commands_count = 0;

void protocol_task_init(api_t* api_array)
{
    api = api_array;
    commands_count = 0;

    while (api[commands_count].command_name != NULL)
    {
        commands_count++;
    }
}

void protocol_task_handle(char* command_string)
{
    if (!command_string)
    {
        return;
    }


    const char* command_name = command_string;
    const char* command_args = NULL;

    char* space_symbol = strchr(command_string, ' ');

    if (space_symbol)
    {
        *space_symbol = '\0';
        command_args = space_symbol + 1;
    }
    else
    {
        command_args = "";
    }

    printf("Command: '%s', Args: '%s'\n", command_name, command_args);

    for (int i = 0; i < commands_count; i++)
    {
        if (strcmp(command_name, api[i].command_name) == 0)
        {
            api[i].command_callback(command_args);
            return;
        }
    }

    printf("Error: Unknown command '%s'\n", command_name);
}
