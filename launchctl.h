#include <xpc/xpc.h>

typedef int cmd_main(xpc_object_t *, int, char **, char **, char **);

// launchctl.c
cmd_main list_cmd;
cmd_main version_cmd;
cmd_main help_cmd;

// start_stop.c
cmd_main stop_cmd;
cmd_main start_cmd;

// print.c
cmd_main print_cmd;

// env.c
cmd_main setenv_cmd;
cmd_main getenv_cmd;

// load.c
cmd_main load_cmd;

// enable.c
cmd_main enable_cmd;

// reboot.c
cmd_main reboot_cmd;

// bootstrap.c
cmd_main bootstrap_cmd;
cmd_main bootout_cmd;

void launchctl_xpc_object_print(xpc_object_t, const char *name, int level);
int launchctl_send_xpc_to_launchd(uint64_t routine, xpc_object_t msg, xpc_object_t *reply);
void launchctl_setup_xpc_dict(xpc_object_t dict);
int launchctl_setup_xpc_dict_for_service_name(char *servicetarget, xpc_object_t dict, const char **name);
void launchctl_print_domain_str(FILE *s, xpc_object_t msg);
xpc_object_t launchctl_parse_load_unload(unsigned int domain, int count, char **list);
