#ifndef __INVMI_INT_FNS_H__
#define __INVMI_INT_FNS_H__

#include <stdint.h>
#include <stdbool.h>

int connect_guest_agent(void);
int64_t get_ksymbol_addr(const char *ksymbol);
int64_t get_kmod_addr(const char *mod_name);
int64_t get_kmod_size(const char *mod_name);
int64_t get_word_size(void);
void load_infos (char *ksym, char *kinfo);

#endif
