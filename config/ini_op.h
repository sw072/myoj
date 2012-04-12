#ifndef _INI_OP_
#define _INI_OP_

#include <unistd.h>
#include <stdio.h>

#define KEY_MAX 128
#define VALUE_MAX 2048

typedef struct _keyval
{
	char key[KEY_MAX];
	char value[VALUE_MAX];
} keyval_t;

typedef struct _section
{
    keyval_t kv;
	struct _section *next;
} ini_section_t, *ini_file_t;

int load_ini_file(char ini_path[], ini_file_t *ini);

int get_profile_string(ini_file_t ini, const char key[], char value[]);

int get_profile_int(ini_file_t ini, const char key[], int *value);

int unload_ini_file(ini_file_t ini);

#endif
