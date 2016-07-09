#ifndef IN_VMI_LINUX_H
#define IN_VMI_LINUX_H

#define PLUGIN_NAME   "in_vmi_linux"
#define DEFAULT_VMI_SOCKET "/tmp/qga.sock"

#define QMP_RECV_SIZE 256

const char *qmp_template = "{ \"execute\": \"get-ksymbol\", \"arguments\": { \"symbol\": \"%s\" } }";
const char *qmp_recv_temp = "{\"return\": {%lld}}";

#endif
