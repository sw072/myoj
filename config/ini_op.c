#include "ini_op.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../trace/trace.h"

static int _valid_section(const char *pstr, char *key, char *value);

int load_ini_file(char ini_path[], ini_file_t *pini)
{
	if(access(ini_path, 0))
	{
	    __TRACE_LN(__TRACE_KEY, "oops : ini file not found");
		return -1;
	}
	if(access(ini_path, 4))
	{
	    __TRACE_LN(__TRACE_KEY, "oops : ini file can't be readed");
		return -1;
	}
	FILE *fp = fopen(ini_path, "r");
	if(!fp)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : ini file open failed");
		return -1;
	}
	/* header node */
	ini_file_t header = (ini_section_t *)malloc(sizeof(ini_section_t));
	memset(header, 0, sizeof(ini_section_t));
	ini_file_t pcur = header;
	/* parse sections */
	char line[KEY_MAX + VALUE_MAX];
	char key[KEY_MAX], value[VALUE_MAX];
	char *ptr;
	while(fgets(line, KEY_MAX + VALUE_MAX, fp) != NULL)
	{
	    if(line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = '\0';
	    if(line[strlen(line) - 1] == '\r') line[strlen(line) - 1] = '\0';
		ptr = line;
		while(*ptr == ' ') ptr++;
		if(*ptr == '#') continue;		/* comment line */
		if(_valid_section(ptr, key, value)) continue;	/* invalid section */
		/* new section */
		pcur->next = (ini_section_t *)malloc(sizeof(ini_section_t));
		assert(pcur->next);
		pcur = pcur->next;
		pcur->next = NULL;
		strcpy(pcur->kv.key, key);
		strcpy(pcur->kv.value, value);
	}
	fclose(fp);
	*pini = header;
	return 0;
}

static int _valid_section(const char *pstr, char *key, char *value)
{
	int ret = 0;
	char *pkey = (char *)pstr;
	char *pval = NULL;
	char *ptr = strstr(pstr, "=");
	if(!ptr)
	{
		return -1;
	}
	/* get value */
	pval = ptr + 1;    /* skip the '=' */
	while(*pval && *pval == ' ') pval++;
	int idx = 0;
	while(*pval)
	{
	    value [idx] = *pval;
	    idx++;
	    pval++;
	}
	value[idx] = '\0';
	/* get key */
	ptr--;          /* skip the '=' */
	while(*ptr == ' ') ptr--;
	idx = 0;
	while(pkey <= ptr)
	{
		if(*pkey == ' ')
		{
			ret = -1;
			break;
		}
		key[idx] = *pkey;
		idx++;
		pkey++;
	}
	key[idx] = '\0';
	return ret;
}

int get_profile_string(ini_file_t ini, const char key[], char value[])
{
	ini_file_t ptr = ini->next;
	while(ptr)
	{
		if(!strcmp(key, ptr->kv.key))
		{
			strcpy(value, ptr->kv.value);
			return 0;
		}
		ptr = ptr->next;
	}
	return -1;
}

int get_profile_int(ini_file_t ini, const char key[], int *value)
{
	char v[32];
	if(get_profile_string(ini, key, v) < 0) return -1;
	if(!(strlen(v) < 9)) return -1;		/* NOTE : MAX is 999999999, in this scenario it is enough*/
	if(v[0] == '\0') *value = 0;
	*value = atoi(v);
	return 0;
}


int unload_ini_file(ini_file_t ini)
{
	assert(ini);
	ini_section_t *ptr = ini;
	while(ini)
	{
		ptr = ini->next;
		free(ini);
		ini = ptr;
	}
	return 0;
}
