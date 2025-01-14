#ifndef __AFL_SNAPSHOT_H__
#define __AFL_SNAPSHOT_H__

#include <asm/mmu.h>
#include <asm/page.h>
#include <linux/auxvec.h>
#include <linux/completion.h>
#include <linux/cpumask.h>
#include <linux/hashtable.h>
#include <linux/kprobes.h>
#include <linux/list.h>
#include <linux/page-flags-layout.h>
#include <linux/rbtree.h>
#include <linux/rwsem.h>
#include <linux/spinlock.h>
#include <linux/threads.h>
#include <linux/types.h>
#include <linux/uprobes.h>
#include <linux/workqueue.h>
#include <linux/acct.h>
#include <linux/aio.h>
#include <linux/audit.h>
#include <linux/binfmts.h>
#include <linux/blkdev.h>
#include <linux/capability.h>
#include <linux/cgroup.h>
#include <linux/cn_proc.h>
#include <linux/compat.h>
#include <linux/compiler.h>
#include <linux/completion.h>
#include <linux/cpu.h>
#include <linux/delayacct.h>
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/freezer.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/ftrace.h>
#include <linux/futex.h>
#undef MODULE
#include <linux/hugetlb.h>
#define MODULE 1
#include <linux/init.h>
#include <linux/iocontext.h>
#include <linux/jiffies.h>
#include <linux/kcov.h>
#include <linux/key.h>
#include <linux/khugepaged.h>
#include <linux/ksm.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/magic.h>
#include <linux/memcontrol.h>
#include <linux/mempolicy.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mmu_notifier.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/nsproxy.h>
#include <linux/oom.h>
#include <linux/pagewalk.h>
#include <linux/perf_event.h>
#include <linux/personality.h>
#include <linux/posix-timers.h>
#include <linux/proc_fs.h>
#include <linux/profile.h>
#include <linux/ptrace.h>
#include <linux/random.h>
#include <linux/rcupdate.h>
#include <linux/rmap.h>
#include <linux/seccomp.h>
#include <linux/security.h>
#include <linux/sem.h>
#include <linux/signalfd.h>
#include <linux/slab.h>
#include <linux/swap.h>
#include <linux/syscalls.h>
#include <linux/sysctl.h>
#include <linux/task_io_accounting_ops.h>
#include <linux/taskstats_kern.h>
#include <linux/tsacct_kern.h>
#include <linux/tty.h>
#include <linux/unistd.h>
#include <linux/uprobes.h>
#include <linux/user-return-notifier.h>
#include <linux/vmacache.h>

#include <asm/cacheflush.h>
#undef MODULE
#include <asm/mmu_context.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>
#include <asm/pgtable_types.h>

#include <asm/tlbflush.h>
#include <asm/tlb.h>
#define MODULE 1
#include <asm/uaccess.h>
#include <linux/list.h>

#include "afl_snapshot.h"
#include "ftrace_util.h"

struct task_data;

// TODO lock VMA restore
struct snapshot_vma {
	unsigned long vm_start;
	unsigned long vm_end;

	bool is_anonymous_private;
	unsigned long prot;

	struct list_head all_vmas_node;
	struct list_head snapshotted_vmas_node;
};

struct snapshot_thread {

  struct task_struct *tsk;

};

struct snapshot_page {

  unsigned long page_base;
  unsigned long page_prot;
  void *        page_data;

  bool has_been_copied;
  bool has_had_pte;
  bool dirty;
  bool in_dirty_list;

  struct hlist_node next;

  struct list_head dirty_list;

};

#define SNAPSHOT_PRIVATE 0x00000001
#define SNAPSHOT_COW 0x00000002
#define SNAPSHOT_NONE_PTE 0x00000010

static inline bool is_snapshot_page_none_pte(struct snapshot_page *sp) {

  return sp->page_prot & SNAPSHOT_NONE_PTE;

}

static inline bool is_snapshot_page_cow(struct snapshot_page *sp) {

  return sp->page_prot & SNAPSHOT_COW;

}

static inline bool is_snapshot_page_private(struct snapshot_page *sp) {

  return sp->page_prot & SNAPSHOT_PRIVATE;

}

static inline void set_snapshot_page_none_pte(struct snapshot_page *sp) {

  sp->page_prot |= SNAPSHOT_NONE_PTE;

}

static inline void set_snapshot_page_private(struct snapshot_page *sp) {

  sp->page_prot |= SNAPSHOT_PRIVATE;

}

static inline void set_snapshot_page_cow(struct snapshot_page *sp) {

  sp->page_prot |= SNAPSHOT_COW;

}

struct open_files_snapshot {
	struct files_struct *files;
	loff_t *offsets;
};

#define SNAPSHOT_HASHTABLE_SZ 0x8

struct snapshot {

  unsigned int  status;
  unsigned long oldbrk;

  struct list_head all_vmas;
  struct list_head snapshotted_vmas;

  struct pt_regs regs;

  struct open_files_snapshot ss_files;

  DECLARE_HASHTABLE(ss_pages, SNAPSHOT_HASHTABLE_SZ);

  struct list_head dirty_pages;

};

#define SNAPSHOT_NONE 0x00000000  // outside snapshot
#define SNAPSHOT_MADE 0x00000001  // in snapshot
#define SNAPSHOT_HAD 0x00000002   // once had snapshot

extern void (*k_flush_tlb_mm_range)(struct mm_struct *mm, unsigned long start,
                                    unsigned long end,
                                    unsigned int  stride_shift,
                                    bool          freed_tables);

extern void (*k_zap_page_range)(struct vm_area_struct *vma, unsigned long start,
                                unsigned long size);

/* The signature of dup_fd was changed in 5.9.0 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
typedef struct files_struct *(*dup_fd_t)(struct files_struct *oldf,
					 unsigned int max_fds, int *errorp);
#define dup_fd(oldf, max_fds, errorp) dup_fd_ptr(oldf, max_fds, errorp)
#else
typedef struct files_struct *(*dup_fd_t)(struct files_struct *oldf,
					 int *errorp);
#define dup_fd(oldf, max_fds, errorp) dup_fd_ptr(oldf, errorp)
#endif
extern dup_fd_t dup_fd_ptr;

typedef void (*put_files_struct_t)(struct files_struct *fs);
extern put_files_struct_t put_files_struct_ptr;
#define put_files_struct put_files_struct_ptr

typedef int (*walk_page_vma_t)(struct vm_area_struct *vma,
			       const struct mm_walk_ops *ops, void *private);
extern walk_page_vma_t walk_page_vma_ptr;
#define walk_page_vma walk_page_vma_ptr

typedef int (*walk_page_range_t)(struct mm_struct *mm, unsigned long start,
				 unsigned long end,
				 const struct mm_walk_ops *ops, void *private);
extern walk_page_range_t walk_page_range_ptr;
#define walk_page_range walk_page_range_ptr

int take_memory_snapshot(struct task_data *data);
int recover_memory_snapshot(struct task_data *data);
int restore_brk(unsigned long old_brk);
void clean_memory_snapshot(struct task_data *data);

#ifdef DEBUG
void dump_memory_snapshot(struct task_data *data);
#endif

int take_files_snapshot(struct task_data *data);
int recover_files_snapshot(struct task_data *data);
void clean_files_snapshot(struct task_data *data);

void recover_threads_snapshot(struct task_data *data);

void do_wp_page_hook(unsigned long ip, unsigned long parent_ip,
		     struct ftrace_ops *op, ftrace_regs_ptr regs);
void page_add_new_anon_rmap_hook(unsigned long ip, unsigned long parent_ip,
				 struct ftrace_ops *op, ftrace_regs_ptr regs);
void __do_munmap_hook(unsigned long ip, unsigned long parent_ip,
		      struct ftrace_ops *op, ftrace_regs_ptr regs);

typedef void (*do_exit_t)(long code);
extern do_exit_t do_exit_orig;
void do_exit_hook(long code);

int  take_snapshot(int config);
int recover_snapshot(void);
void clean_snapshot(void);
int  exit_snapshot(void);

void exclude_vmrange(unsigned long start, unsigned long end);
void include_vmrange(unsigned long start, unsigned long end);

#endif

