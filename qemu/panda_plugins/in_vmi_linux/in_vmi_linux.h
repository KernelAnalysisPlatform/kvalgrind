#ifndef IN_VMI_LINUX_H
#define IN_VMI_LINUX_H

#define PLUGIN_NAME   "in_vmi_linux"
#define DEFAULT_VMI_SOCKET "/tmp/qga.sock"

#define QMP_RECV_SIZE 256

#define QMP_TEMPLATE(comm) "{ \"execute\": \"" #comm "\" }\n"
#define QMP_TEMPLATE1(comm) "{ \"execute\": \"" #comm "\", \"arguments\": { \"symbol\": \"%s\" } }\n"
#define QMP_RECV_TEMP "{\"return\": {%lld}}\n"

#endif
