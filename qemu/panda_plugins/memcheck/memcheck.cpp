/*
 * Memcheck plugin for kernel module.
 *
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

#include "rr_log.h"

#include "../callstack_instr/callstack_instr.h"
#include "../in_vmi_linux/in_vmi_linux.h"

#include "pandalog.h"
#include "panda/panda_addr.h"
#include <glib.h>
#include <stdint.h>
#include <stdio.h>

bool init_plugin(void *);
void uninit_plugin(void *);

int virt_mem_write(CPUState *env, target_ulong pc, target_ulong addr, target_ulong size, void *buf);
int virt_mem_read(CPUState *env, target_ulong pc, target_ulong addr, target_ulong size, void *buf);
int before_block_exec(CPUState *env, TranslationBlock *tb);

}

#include <map>
#include <set>
#include <stack>
#include <queue>
#include "../taint2/taint2.h"
#include "../taint2/taint2_ext.h"
#include "memcheck.h"
#include "../common/prog_point.h"
#include "../callstack_instr/callstack_instr_ext.h"
#include "../in_vmi_linux/in_vmi_linux_ext.h"

static const char *alloc_sym_name, *free_sym_name, *kmod_name;
int64_t alloc_guest_addr, free_guest_addr, kmod_addr, kmod_size;
static unsigned word_size;
static std::stack<alloc_info> alloc_stacks;
static std::stack<free_info> free_stacks;
static range_set alloc_now; // Allocated now.
static merge_range_set alloc_ever; // Allocated ever.
static std::map<target_ulong, target_ulong> valid_ptrs; //ptr -> valid(acllocated) address
static std::map<target_ulong, int64_t> invalid_ptrs;    //ptr -> invalid(freed) address
static std::queue<target_ulong> invalid_queue;    //ptr -> invalid(freed) address
static std::queue<read_info> bad_read_queue; //read operations from invalid(freed) address
static const unsigned safety_window = 1000000;

void taint_change(Addr a, uint64_t size)
{
  /* TODO: Implement uninitialized value detector by using taint analysis */
}

static int virt_mem_access(CPUState *env, target_ulong pc, target_ulong addr, target_ulong size, void *buf, int is_write);
void process_ret(CPUState *env, target_ulong func);

static bool in_kernel_module(target_ulong pc)
{
  return kmod_addr <= pc && pc < kmod_addr + kmod_size;
}

/*
 *  - UAF check
 *    alloc and free function call is traced by BLOCK_EXEC callback method.
 *    When these functions are returned and back to caller function, then
 *    callstack_instr callback is invoked and change pointer statuses.
 *
 */

static bool inside_memop()
{
  return !(alloc_stacks.empty() && free_stacks.empty());
}

void process_ret(CPUState *env, target_ulong func)
{
  if (!in_kernel_module(env->eip)) return;

  if (!alloc_stacks.empty() && env->eip == alloc_stacks.top().retaddr) {
      alloc_info info = alloc_stacks.top();
      target_ulong addr = env->regs[R_EAX];
          if (addr != 0) {
              alloc_now.insert(info.heap, addr, addr + info.size);
              alloc_ever.insert(info.heap, addr, addr + info.size);
          }
      alloc_stacks.pop();
  } else if (!free_stacks.empty() && env->eip == free_stacks.top().retaddr) {
      free_info info = free_stacks.top();
      if (info.addr > 0 && alloc_ever.contains(info.addr)) {
          if (!alloc_now.contains(info.addr)) {
            /* The address that is once allocated then freed(alloc_now doesn't contain). */
              if (!inside_memop())
                  printf("DOUBLE FREE @ {%lx}! PC %lx\n", info.addr, env->eip);
          } else if (free_stacks.size() == 1) {
            /* XXX: Is it possible that free function is nested? */
              range_info &ri = alloc_now[info.addr];
              for (auto it = ri.valid_ptrs.begin(); it != ri.valid_ptrs.end(); it++) {
                  printf("Invalidating pointer @ %lx\n", *it);
                  // *it is the location of a pointer into the freed range
                  if (alloc_now.contains(*it)) {
                      invalid_queue.push(*it);
                  }
                  invalid_ptrs[*it] = rr_get_guest_instr_count();
                  valid_ptrs.erase(*it);
              }
              alloc_now.remove(info.addr);
          }
      }
      free_stacks.pop();
  }
}

static int virt_mem_access(CPUState *env, target_ulong pc, target_ulong addr,
                          target_ulong size, void *buf, int is_write)
{
  if (!in_kernel_module(pc)) return 0;

  if (!inside_memop()) {
       if (alloc_ever.contains(addr)
               && !alloc_now.contains(addr)) {
           printf("USE AFTER FREE %s @ {%lx}! PC %lx\n",
                   is_write ? "WRITE" : "READ", addr, pc);
           return 0;
       }

       if (size == word_size) {
           target_ulong loc = addr; // Pointer location
           // Pointer value; should be address inside valid range
           target_ulong val = *(uint32_t *)buf;
           // Might be writing a pointer. Track.
           if (is_write) {
               if (alloc_now.contains(val)) { // actually creating pointer.
                   printf("Creating pointer to %lx @ %lx.\n", val, loc);
                   alloc_now[val].valid_ptrs.insert(loc);
                   try { valid_ptrs[loc] = val; } catch (int e) {}
               } else if (alloc_ever.contains(val)) {
                   // Oops! We wrote an invalid pointer.
                   printf("Writing invalid pointer to %lx @ %lx.\n", val, loc);
                   invalid_ptrs[loc] = rr_get_guest_instr_count();
               }
           } else if (env->regs[R_ESP] != loc) { // Reading a pointer. Ignore stack reads.
               // Leave safety window.
               if (invalid_ptrs.count(loc) > 0 &&
                       rr_get_guest_instr_count() - invalid_ptrs[loc] > safety_window &&
                       val != 0) {
                   bad_read_queue.push(read_info(pc, loc, val));
               }
           }
       }
   }
   return 0;
}

int virt_mem_write(CPUState *env, target_ulong pc, target_ulong addr,
                  target_ulong size, void *buf)
{
  return virt_mem_access(env, pc, addr, size, buf, 1);
}

int virt_mem_read(CPUState *env, target_ulong pc, target_ulong addr,
                  target_ulong size, void *buf)
{
  return virt_mem_access(env, pc, addr, size, buf, 0);
}

// Assumes target+host have same endianness.
static target_ulong get_word(CPUState *env, target_ulong addr)
{
    target_ulong result = 0;
    panda_virtual_memory_rw(env, addr, (uint8_t *)&result, word_size, 0);
    return result;
}

// Returns [esp + word_size*offset_number]
static target_ulong get_stack(CPUState *env, int offset_number)
{
    return get_word(env, env->regs[R_ESP] + word_size * offset_number);
}

int before_block_exec(CPUState *env, TranslationBlock *tb)
{
  if (!in_kernel_module(tb->pc)) return 0;

  // Clear queue of potential bad reads.
  while (bad_read_queue.size() > 0) {
      read_info& ri = bad_read_queue.front();
      if (get_word(env, ri.loc) == ri.val) { // Still invalid.
          printf("READING INVALID POINTER %lx @ %lx!! PC %lx\n", ri.val, ri.loc, ri.pc);
      }
      bad_read_queue.pop();
  }

  // Clear queue of potential dangling pointers.
  while (invalid_queue.size() > 0) {
      target_ulong loc = invalid_queue.front();

      if (invalid_ptrs.count(loc) == 0 || !alloc_now.contains(loc)) {
          // Pointer has been overwritten or deallocated; not dangling.
          invalid_queue.pop();
          continue;
      }
      if (rr_get_guest_instr_count() - invalid_ptrs[loc] <= safety_window) {
          // Inside safety window still.
          break;
      }

      // Outside safety window and pointer is still dangling. Report.
      printf("POINTER RETENTION to %lx @ %lx!\n", get_word(env, loc), loc);
      invalid_queue.pop();
  }
  /* Now pc address is in the kernel module. */
  if (tb->pc == alloc_guest_addr) {
    alloc_info info;
    info.retaddr = get_stack(env, 0);
    info.heap = get_stack(env, 1);
    //info.size = get_stack(env, 3);
    alloc_stacks.push(info);
  }
  else if (tb->pc == free_guest_addr) {
    free_info info;
    info.retaddr = get_stack(env, 0);
    info.heap = get_stack(env, 1);
    info.addr = get_stack(env, 3);
    free_stacks.push(info);
  }
  return 0;
}

static void init_vmi()
{
  char enter;
  if (connect_guest_agent()) {
    alloc_guest_addr = get_ksymbol_addr(alloc_sym_name);
    free_guest_addr = get_ksymbol_addr(free_sym_name);
    kmod_addr = get_kmod_addr(kmod_name);
    kmod_size = get_kmod_size(kmod_name);
    word_size = get_word_size();
    fprintf(stderr, "kmalloc:0x%016x, kfree:0x%016x, word:%d\n%s:0x%16x, %d\n",
            alloc_guest_addr, free_guest_addr, word_size, kmod_name, kmod_addr, kmod_size);
  }
}

//load_plugin ./qemu/x86_64-softmmu/panda_plugins/panda_memcheck.so

bool init_plugin(void *self)
{
  printf("Initializing memcheck plugin\n");
  panda_require("taint2");
  assert(init_taint2_api());
  panda_require("callstack_instr");
  assert (init_callstack_instr_api());
  panda_require("in_vmi_linux");
  assert(init_in_vmi_linux_api());
  PPP_REG_CB("callstack_instr", on_ret, process_ret);
  panda_enable_memcb();
  panda_cb pcb;
  pcb.virt_mem_write = virt_mem_write;
  panda_register_callback(self, PANDA_CB_VIRT_MEM_WRITE, pcb);
  pcb.virt_mem_read = virt_mem_read;
  panda_register_callback(self, PANDA_CB_VIRT_MEM_READ, pcb);
  pcb.before_block_exec = before_block_exec;
  panda_register_callback(self, PANDA_CB_BEFORE_BLOCK_EXEC, pcb);

  /* Parse options */
  panda_arg_list *args = panda_get_args("memcheck");
  alloc_sym_name = g_strdup(panda_parse_string(args, "alloc", "__kmalloc"));
  free_sym_name  = g_strdup(panda_parse_string(args, "free", "kfree"));
  kmod_name      = g_strdup(panda_parse_string(args, "module", ""));
  /* Init VMI functions */
  init_vmi();
  return true;
}

void uninit_plugin(void *self)
{

}
