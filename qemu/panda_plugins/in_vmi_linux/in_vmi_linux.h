#ifndef IN_VMI_LINUX_H
#define IN_VMI_LINUX_H

#define PLUGIN_NAME   "in_vmi_linux"
#define DEFAULT_VMI_SOCKET "/tmp/qga.sock"

#define QMP_RECV_SIZE 256

#define QMP_TEMPLATE(comm) "{ \"execute\": \"comm\" }"
#define QMP_TEMPLATE1(comm) "{ \"execute\": \"comm\", \"arguments\": { \"symbol\": \"%s\" } }"
#define QMP_RECV_TEMP "{\"return\": {%lld}}"

int connect_guest_agent();
int64_t get_ksymbol_addr(const char *ksymbol);
int64_t get_kmod_addr(const char *mod_name);
int64_t get_kmod_size(const char *mod_name);
int64_t get_word_size();
#endif
