/*
 * In-VMI plugin.
 * This plugin communicate with agent in guest os.
 * So firstly, a qemu-ga must be loaded in guest os.
 *
 * Author:
 *  Ren Kimura               rkx1209dev@gmail.com
 *
 * This work is licensed under the terms of the GNU GPL, version 3.
 * See the COPYING file in the top-level directory.
 *
 */

#define __STDC_FORMAT_MACROS

extern "C" {

#include "config.h"
#include "qemu-common.h"

#include "panda_common.h"
#include "panda_plugin.h"
#include "panda_plugin_plugin.h"

#include "pandalog.h"
#include "panda/panda_addr.h"
#include "in_vmi_linux.h"

#include <glib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

bool init_plugin(void *);
void uninit_plugin(void *);
int connect_guest_agent();
int64_t get_ksymbol_addr(const char *ksymbol);
struct module *get_kmod(const char *mod_name);
bool sym_in_kmod(struct module *kmod, const char *symbol);

}

#include <map>
#include <set>
#include <stack>
#include <queue>

const char *socket_path;
int agent_sock = -1;

int connect_guest_agent()
{
  struct sockaddr_un agent_addr;
  agent_addr.sun_family = PF_UNIX;
  strcpy(agent_addr.sun_path, socket_path);

  errno = 0;
  if((agent_sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1){
    perror("in_vmi_linux: agent socket failure");
    exit(EXIT_FAILURE);
  }

  if(connect(agent_sock, (struct sockaddr *)&agent_addr, sizeof(agent_addr)) == -1){
    perror("in_vmi_linux: connection to agent failuer\nPlease launch agent in your guest os firstly");
    close(agent_sock);
    return EAGAIN;
  }
  return agent_sock;
}

void send_qmp(int sock, char *request, char *response, int resp_size)
{
  int write_num,read_num;
  if (sock == -1) {
    perror("send_qmp: connection to guest agent not exist.");
  }
  if ((write_num = write(sock, request, strlen(request))) != -1){
    fprintf(stderr,"sended qmp:\t%s\n",request);
    errno = 0;
    if ((read_num = read(sock, response, resp_size)) != -1){
      fprintf(stderr,"received qmp:\t%s\n",response);
    } else{
      perror("read failure");
    }
  } else{
    perror("write failure");
  }

}

/* TODO: Following functions are bit ugly and too duplicated. */
int64_t get_ksymbol_addr(const char *ksymbol)
{
  char *ksym_qmp;
  char recv_qmp[QMP_RECV_SIZE];
  int64_t addr;

  ksym_qmp = (char *)malloc(strlen(QMP_TEMPLATE(get-ksymbol)) + strlen(ksymbol) - 2 + 1);
  sprintf(ksym_qmp, QMP_TEMPLATE(get-ksymbol), ksymbol);

  send_qmp(agent_sock, ksym_qmp, recv_qmp, QMP_RECV_SIZE);

  sscanf(recv_qmp, QMP_RECV_TEMP, &addr);

  free(ksym_qmp);

  return addr;
}

int64_t get_kmod_addr(const char *mod_name)
{
  char *ksym_qmp;
  char recv_qmp[QMP_RECV_SIZE];
  int64_t addr;

  ksym_qmp = (char *)malloc(strlen(QMP_TEMPLATE(kmod-addr)) + strlen(mod_name) - 2 + 1);
  sprintf(ksym_qmp, QMP_TEMPLATE(kmod-addr), mod_name);

  send_qmp(agent_sock, ksym_qmp, recv_qmp, QMP_RECV_SIZE);

  sscanf(recv_qmp, QMP_RECV_TEMP, &addr);

  free(ksym_qmp);

  return addr;
}


int64_t get_kmod_size(const char *mod_name)
{
  char *ksym_qmp;
  char recv_qmp[QMP_RECV_SIZE];
  int64_t size;

  ksym_qmp = (char *)malloc(strlen(QMP_TEMPLATE(kmod-size)) + strlen(mod_name) - 2 + 1);
  sprintf(ksym_qmp, QMP_TEMPLATE(kmod-size), mod_name);

  send_qmp(agent_sock, ksym_qmp, recv_qmp, QMP_RECV_SIZE);

  sscanf(recv_qmp, QMP_RECV_TEMP, &size);

  free(ksym_qmp);

  return size;
}

int64_t get_word_size()
{
  char *ksym_qmp;
  char recv_qmp[QMP_RECV_SIZE];
  int64_t size;

  ksym_qmp = (char *)malloc(strlen(QMP_TEMPLATE(word-size)) - 2 + 1);
  sprintf(ksym_qmp, QMP_TEMPLATE(word-size));

  send_qmp(agent_sock, ksym_qmp, recv_qmp, QMP_RECV_SIZE);

  sscanf(recv_qmp, QMP_RECV_TEMP, &size);

  free(ksym_qmp);

  return size;
}
// struct module *get_kmod(const char *mod_name)
// {
//   struct module *mod_head, *mod_entry;
//   struct list_head *p;
//   mod_head = (struct module *)get_ksymbol_addr("modules");
//   list_for_each(p, &mod_head->list) {
//     mod_entry = list_entry(p, struct module, list);
//     if (strcmp(mod_entry->name, mod_name) == 0) {
//       return mod_entry;
//     }
//   }
//   return NULL;
// }
//
// /* Does the symbol belong to specified kernel module? */
// bool sym_in_kmod(struct module *kmod, const char *symbol)
// {
//   /* Parse Elf_Sym in kernel module strcture. */
//   Elf_Sym *sym_entry;
//   int s;
//   for (s = 0; s < kmod->num_symtab; s++) {
//     sym_entry = &kmod->symtab[s];
//     char *ksym = kmod->strtab + sym_entry->st_name;
//     if (memcmp(symbol, ksym, sym_entry->st_size) == 0) {
//       return true;
//     }
//   }
//   return false;
// }

bool init_plugin(void *self)
{
  /* Parse options */
  panda_arg_list *args = panda_get_args(PLUGIN_NAME);
  socket_path = g_strdup(panda_parse_string(args, "soc", DEFAULT_VMI_SOCKET));
  return true;
}

void uninit_plugin(void *self)
{
  return;
}
