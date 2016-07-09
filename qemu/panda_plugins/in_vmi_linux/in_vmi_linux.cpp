/*
 * In-VMI plugin.
 * This plugin communicate with agent in guest os.
 * So firstly, a qemu-ga must be loaded in guest os.
 *
 * Author:
 *  Ren Kimura               rkx1209dev@gmail.com
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
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
    exit(EXIT_FAILURE);
  }
  return agent_sock;
}

int64_t get_ksymbol_addr(const char *ksymbol)
{
  char *ksym_qmp;
  char recv_qmp[QMP_RECV_SIZE];
  int write_num,read_num;
  int64_t addr;

  if (agent_sock == -1) {
    fprintf(stderr, "get_ksymbol_addr: connection to guest agent not exist.\n");
    exit(EXIT_FAILURE);
  }

  ksym_qmp = (char *)malloc(strlen(qmp_template) + strlen(ksymbol) - 2 + 1);
  sprintf(ksym_qmp, qmp_template, ksymbol);

  if ((write_num = write(agent_sock, ksym_qmp, strlen(ksym_qmp))) != -1){
    fprintf(stderr,"sended qmp:\t%s\n",ksym_qmp);
    errno = 0;
    if ((read_num = read(agent_sock, recv_qmp, QMP_RECV_SIZE)) != -1){
      fprintf(stderr,"received qmp:\t%s\n",recv_qmp);
    } else{
      perror("read failure");
    }
  } else{
    perror("write failure");
  }
  sscanf(recv_qmp, qmp_recv_temp, &addr);

  free(ksym_qmp);

  return addr;
}

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
