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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

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
int connect_guest_agent(void);
int64_t get_ksymbol_addr(const char *ksymbol);
int64_t get_kmod_addr(const char *mod_name);
int64_t get_kmod_size(const char *mod_name);
int64_t get_word_size(void);
void load_infos (char *ksym, char *kinfo);

}

#include <map>
#include <set>
#include <stack>
#include <queue>
#include <string>

std::map<std::string, int64_t> ksyms,maddr,msize;
const char *socket_path;
int agent_sock = -1;

int __connect_guest_agent(void)
{
  struct sockaddr_un agent_addr;
  agent_addr.sun_family = PF_UNIX;
  strcpy(agent_addr.sun_path, socket_path);

  errno = 0;
  if((agent_sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1){
    fprintf(stderr, "in_vmi_linux: agent socket failure\n");
    exit(EXIT_FAILURE);
  }

  if(connect(agent_sock, (struct sockaddr *)&agent_addr, sizeof(agent_addr)) == -1){
    fprintf(stderr, "in_vmi_linux: connection to agent failuer\nPlease launch agent in your guest os firstly\n");
    close(agent_sock);
    return EAGAIN;
  }
  return agent_sock;
}


void __load_infos (char *ksym, char *kinfo)
{
  FILE *sym_f,*info_f;
  char buf[100] = {0};
  int64_t addr,size;
  fprintf(stderr, "loading local *.kvals\n");
  sym_f = fopen(ksym, "r");
  info_f = fopen(kinfo, "r");
  while (fscanf(sym_f, "%s 0x%16llx", buf, &addr) != EOF) {
    std::string sym = buf;
    //fprintf(stderr, "%s 0x%16llx\n", sym.c_str(), addr);
    ksyms[sym] = addr;
  }
  while (fscanf(info_f, "%s 0x%16llx %d", buf, &addr, &size) != EOF) {
    std::string sym = buf;
    //fprintf(stderr, "%s 0x%16llx %d\n", sym.c_str(), addr, size);
    maddr[sym] = addr;
    msize[sym] = size;
  }
}

void send_qmp(int sock, char *request, char *response, int resp_size)
{
  int write_num,read_num;
  if (sock == -1) {
    perror("send_qmp: connection to guest agent not exist.");
  }
  if ((write_num = write(sock, request, strlen(request))) != -1){
    fprintf(stderr,"sended qmp(%d byte, %d len): %s\n",write_num, strlen(request), request);
    errno = 0;
    /* TODO: When waiting QMP response, read function is blocked and guest os is freezed..
     * So these operations must be multi-threaded.
     */
    // if ((read_num = read(sock, response, resp_size)) != -1){
    //   fprintf(stderr,"received qmp:\t%s\n",response);
    // } else{
    //   perror("read failure");
    // }
  } else{
    perror("write failure");
  }

}

/* TODO: Following functions are bit ugly and too duplicated. */
int64_t __get_ksymbol_addr(const char *ksymbol)
{
  char *ksym_qmp;
  char recv_qmp[QMP_RECV_SIZE];
  int64_t addr;

  // ksym_qmp = (char *)malloc(strlen(QMP_TEMPLATE1(get-ksymbol)) + strlen(ksymbol) - 2 + 1);
  // sprintf(ksym_qmp, QMP_TEMPLATE1(get-ksymbol), ksymbol);
  //
  // send_qmp(agent_sock, ksym_qmp, recv_qmp, QMP_RECV_SIZE);
  //
  // sscanf(recv_qmp, QMP_RECV_TEMP, &addr);
  //
  // free(ksym_qmp);

  //return addr;
  std::string ksymbol_s = ksymbol;
  return ksyms[ksymbol_s];
}

int64_t __get_kmod_addr(const char *mod_name)
{
  char *ksym_qmp;
  char recv_qmp[QMP_RECV_SIZE];
  int64_t addr;

  // ksym_qmp = (char *)malloc(strlen(QMP_TEMPLATE1(kmod-addr)) + strlen(mod_name) - 2 + 1);
  // sprintf(ksym_qmp, QMP_TEMPLATE1(kmod-addr), mod_name);
  //
  // send_qmp(agent_sock, ksym_qmp, recv_qmp, QMP_RECV_SIZE);
  //
  // sscanf(recv_qmp, QMP_RECV_TEMP, &addr);
  //
  // free(ksym_qmp);

  //return addr;
  std::string mod_s = mod_name;
  return maddr[mod_s];
}


int64_t __get_kmod_size(const char *mod_name)
{
  char *ksym_qmp;
  char recv_qmp[QMP_RECV_SIZE];
  int64_t size;

  // ksym_qmp = (char *)malloc(strlen(QMP_TEMPLATE1(kmod-size)) + strlen(mod_name) - 2 + 1);
  // sprintf(ksym_qmp, QMP_TEMPLATE1(kmod-size), mod_name);
  //
  // send_qmp(agent_sock, ksym_qmp, recv_qmp, QMP_RECV_SIZE);
  //
  // sscanf(recv_qmp, QMP_RECV_TEMP, &size);
  //
  // free(ksym_qmp);

  //return size;
  std::string mod_s = mod_name;
  return msize[mod_s];
}

int64_t __get_word_size(void)
{
  char *ksym_qmp;
  char recv_qmp[QMP_RECV_SIZE];
  int64_t size;

  // ksym_qmp = (char *)malloc(strlen(QMP_TEMPLATE(word-size)) + 1);
  // sprintf(ksym_qmp, QMP_TEMPLATE(word-size));
  //
  // send_qmp(agent_sock, ksym_qmp, recv_qmp, QMP_RECV_SIZE);
  //
  // sscanf(recv_qmp, QMP_RECV_TEMP, &size);
  //
  // free(ksym_qmp);
  //
  // return size;
  return 4;
}

int connect_guest_agent(void)
{
  return __connect_guest_agent();
}
void load_infos (char *ksym, char *kinfo)
{
  __load_infos(ksym, kinfo);
}
int64_t get_ksymbol_addr(const char *ksymbol)
{
  return __get_ksymbol_addr(ksymbol);
}
int64_t get_kmod_addr(const char *mod_name)
{
  return __get_kmod_addr(mod_name);
}
int64_t get_kmod_size(const char *mod_name)
{
  return __get_kmod_size(mod_name);
}
int64_t get_word_size(void)
{
  return __get_word_size();
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
