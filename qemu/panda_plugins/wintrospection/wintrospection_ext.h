#ifndef __WINTROSPECTION_EXT_H__
#define __WINTROSPECTION_EXT_H__

/*
 * DO NOT MODIFY. This file is automatically generated by scripts/apigen.py,
 * based on the <plugin>_int.h file in your plugin directory.
 */

#include <dlfcn.h>
#include "panda_plugin.h"

typedef uint32_t(*get_current_proc_t)(CPUState *env);
static get_current_proc_t __get_current_proc = NULL;
static inline uint32_t get_current_proc(CPUState *env);
static inline uint32_t get_current_proc(CPUState *env){
    assert(__get_current_proc);
    return __get_current_proc(env);
}
typedef uint32_t(*get_pid_t)(CPUState *env, uint32_t eproc);
static get_pid_t __get_pid = NULL;
static inline uint32_t get_pid(CPUState *env, uint32_t eproc);
static inline uint32_t get_pid(CPUState *env, uint32_t eproc){
    assert(__get_pid);
    return __get_pid(env,eproc);
}
typedef void(*get_procname_t)(CPUState *env, uint32_t eproc, char *name);
static get_procname_t __get_procname = NULL;
static inline void get_procname(CPUState *env, uint32_t eproc, char *name);
static inline void get_procname(CPUState *env, uint32_t eproc, char *name){
    assert(__get_procname);
    return __get_procname(env,eproc,name);
}
typedef HandleObject *(*get_handle_object_t)(CPUState *env, uint32_t eproc, uint32_t handle);
static get_handle_object_t __get_handle_object = NULL;
static inline HandleObject * get_handle_object(CPUState *env, uint32_t eproc, uint32_t handle);
static inline HandleObject * get_handle_object(CPUState *env, uint32_t eproc, uint32_t handle){
    assert(__get_handle_object);
    return __get_handle_object(env,eproc,handle);
}
typedef char *(*get_handle_object_name_t)(CPUState *env, HandleObject *ho);
static get_handle_object_name_t __get_handle_object_name = NULL;
static inline char * get_handle_object_name(CPUState *env, HandleObject *ho);
static inline char * get_handle_object_name(CPUState *env, HandleObject *ho){
    assert(__get_handle_object_name);
    return __get_handle_object_name(env,ho);
}
typedef char *(*get_handle_name_t)(CPUState *env, uint32_t eproc, uint32_t handle);
static get_handle_name_t __get_handle_name = NULL;
static inline char * get_handle_name(CPUState *env, uint32_t eproc, uint32_t handle);
static inline char * get_handle_name(CPUState *env, uint32_t eproc, uint32_t handle){
    assert(__get_handle_name);
    return __get_handle_name(env,eproc,handle);
}
typedef char *(*get_objname_t)(CPUState *env, uint32_t obj);
static get_objname_t __get_objname = NULL;
static inline char * get_objname(CPUState *env, uint32_t obj);
static inline char * get_objname(CPUState *env, uint32_t obj){
    assert(__get_objname);
    return __get_objname(env,obj);
}
typedef uint32_t(*get_handle_table_entry_t)(CPUState *env, uint32_t pHandleTable, uint32_t handle);
static get_handle_table_entry_t __get_handle_table_entry = NULL;
static inline uint32_t get_handle_table_entry(CPUState *env, uint32_t pHandleTable, uint32_t handle);
static inline uint32_t get_handle_table_entry(CPUState *env, uint32_t pHandleTable, uint32_t handle){
    assert(__get_handle_table_entry);
    return __get_handle_table_entry(env,pHandleTable,handle);
}
typedef char *(*get_file_obj_name_t)(CPUState *env, uint32_t fobj);
static get_file_obj_name_t __get_file_obj_name = NULL;
static inline char * get_file_obj_name(CPUState *env, uint32_t fobj);
static inline char * get_file_obj_name(CPUState *env, uint32_t fobj){
    assert(__get_file_obj_name);
    return __get_file_obj_name(env,fobj);
}
typedef char *(*read_unicode_string_t)(CPUState *env, uint32_t pUstr);
static read_unicode_string_t __read_unicode_string = NULL;
static inline char * read_unicode_string(CPUState *env, uint32_t pUstr);
static inline char * read_unicode_string(CPUState *env, uint32_t pUstr){
    assert(__read_unicode_string);
    return __read_unicode_string(env,pUstr);
}
#define API_PLUGIN_NAME "wintrospection"
#define IMPORT_PPP(module, func_name) { \
 __##func_name = (func_name##_t) dlsym(module, #func_name); \
 char *err = dlerror(); \
 if (err) { \
    printf("Couldn't find %s function in library %s.\n", #func_name, API_PLUGIN_NAME); \
    printf("Error: %s\n", err); \
    return false; \
 } \
}
static inline bool init_wintrospection_api(void);static inline bool init_wintrospection_api(void){
    void *module = panda_get_plugin_by_name("panda_" API_PLUGIN_NAME ".so");
    if (!module) {
        printf("In trying to add plugin, couldn't load %s plugin\n", API_PLUGIN_NAME);
        return false;
    }
    dlerror();
IMPORT_PPP(module, get_current_proc)
IMPORT_PPP(module, get_pid)
IMPORT_PPP(module, get_procname)
IMPORT_PPP(module, get_handle_object)
IMPORT_PPP(module, get_handle_object_name)
IMPORT_PPP(module, get_handle_name)
IMPORT_PPP(module, get_objname)
IMPORT_PPP(module, get_handle_table_entry)
IMPORT_PPP(module, get_file_obj_name)
IMPORT_PPP(module, read_unicode_string)
return true;
}

#undef API_PLUGIN_NAME
#undef IMPORT_PPP

#endif
