#include "symbols.h"

const char *quota_name(quota_type_t type)
{
    static const char * quota_name_str[] = {"QUOTA_WALLCLOCK ",
	"QUOTA_CPUTIME", "QUOTA_MEMORY", "QUOTA_OUTPUT",
	"QUOTA_NUM"};
	return quota_name_str[type];
}

const char * event_name(event_type_t type)
{
    static const char *event_name_str[] = {"	EVENT_SYSTEM_CALL",
	"EVENT_SYSTEM_CALL_RETURN",	"EVENT_QUOTA", "EVENT_SIGNAL",
	"EVENT_EXIT"};
	return event_name_str[type];
}

const char *action_name(action_type_t type)
{
    static const char * action_name_str[] = {"ACTION_CONTINUE",
                                                                    "ACTION_KILL", "ACTION_EXIT"};
    return action_name_str[type];
}
