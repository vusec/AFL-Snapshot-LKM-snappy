#ifndef __AFL_SNAPSHOT_TASK_DATA_H__
#define __AFL_SNAPSHOT_TASK_DATA_H__

#include "snapshot.h"
#include <linux/list.h>
#include <linux/sched.h>

struct vmrange {
	unsigned long start;
	unsigned long end;

	struct list_head node;
};

struct task_data {
	const struct task_struct *tsk;

	struct snapshot ss;

	struct list_head allowlist, blocklist;
	int config;

	struct list_head list;
	struct rcu_head rcu;
};

struct task_data *get_task_data(const struct task_struct *tsk);
struct task_data *ensure_task_data(const struct task_struct *tsk);
void              remove_task_data(struct task_data *data);

static inline void clear_snapshot(struct task_data *data) {

  data->ss.status &= ~SNAPSHOT_MADE;

}

static inline void set_had_snapshot(struct task_data *data) {

  data->ss.status |= SNAPSHOT_HAD;

}

static inline void set_snapshot(struct task_data *data) {

  data->ss.status |= SNAPSHOT_MADE;

}

static inline bool have_snapshot(struct task_data *data) {

  return !!(data->ss.status & SNAPSHOT_MADE);

}

static inline bool had_snapshot(struct task_data *data) {

  return !!(data->ss.status & SNAPSHOT_HAD);

}

#endif

