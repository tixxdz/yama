/*
 * Yama Linux Security Module
 *
 * Author: Kees Cook <keescook@chromium.org>
 *         Djalal Harouni
 *
 *
 * Copyright (C) 2010 Canonical, Ltd.
 * Copyright (C) 2011 The Chromium OS Authors.
 * Copyright (c) 2016 Djalal Harouni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */

/*
 * Internal YAMA flags
 * Always order flags by less restrictive to more restrictive ones.
 */
#define YAMA_MOD_HARDEN			0x00000001
#define YAMA_MOD_HARDEN_STRICT		0x00000002

#define YAMA_OPTS_ALL					\
	((unsigned long) (YAMA_MOD_HARDEN |		\
			  YAMA_MOD_HARDEN_STRICT))

struct yama_filter;
struct yama_task;

int yama_filter_calculate_flags(struct yama_filter *filter,
				unsigned long op, unsigned long flag,
				unsigned long *result);

int yama_filter_access(struct yama_filter *filter,
		       unsigned long op, unsigned long flag);

/* Returns true if extact match */
bool yama_filter_flags_match(struct yama_filter *filter, unsigned long flags);

int yama_filter_op_to_flag(unsigned long op, unsigned long value,
			   unsigned long *rvalue);
int yama_task_is_op_set(struct yama_task *yama_tsk, unsigned long op);

struct yama_filter *init_yama_filter(unsigned long flags);
struct yama_filter *lookup_yama_filter(unsigned long flags);
void insert_yama_filter(struct yama_filter *filter);
void remove_yama_filter(struct yama_filter *filter);

void delayed_reclaim_yama_filters(bool reclaim, bool remove);

int init_yama_tasks_hash(void);
void destroy_yama_tasks_hash(void);

struct yama_task *get_yama_task(struct task_struct *tsk);
void put_yama_task(struct yama_task *yama_tsk);

struct yama_task *init_yama_task(struct task_struct *tsk,
				 struct yama_filter *filter);
struct yama_task *give_me_yama_task(struct task_struct *tsk);
struct yama_task *lookup_yama_task(struct task_struct *tsk);
int insert_yama_task(struct yama_task *yama_tsk);

struct yama_filter *get_yama_filter(struct yama_filter *filter);
void put_yama_filter(struct yama_filter *filter, bool *reclaim);

struct yama_filter *get_yama_filter_of_task(struct yama_task *yama_tsk);
void put_yama_filter_of_task(struct yama_task *yama_tsk, bool reclaim);

int update_yama_task_filter(struct yama_task *yama_tsk, unsigned long flags);
