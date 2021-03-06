/*
 * Yama Linux Security Module
 *
 * Author: Djalal Harouni
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

#include <linux/ctype.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/string_helpers.h>
#include <linux/task_work.h>
#include <linux/prctl.h>
#include <linux/security.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include "yama_filter.h"

int yama_tasks_init(void)
{
	return init_yama_tasks_hash();
}

void yama_tasks_clean(void)
{
	return destroy_yama_tasks_hash();
}

static int yama_set_filter(struct yama_task *yama_tsk, unsigned long op,
			   unsigned long flag, unsigned long value)
{
	int ret = -EINVAL;
	bool rm_old = false;
	unsigned long new_flags = 0;
	struct yama_filter *old;

	old = get_yama_filter_of_task(yama_tsk);
	if (old) {
		ret = yama_filter_access(old, op, flag);
		if (ret < 0)
			goto out;

		ret = yama_filter_calculate_flags(old, op, flag, &new_flags);
		if (ret < 0)
			goto out;

		/* Nothing to do if flags did not change */
		if (yama_filter_flags_match(old, new_flags)) {
			ret = 0;
			goto out;
		}
	} else
		new_flags = flag;

	ret = update_yama_task_filter(yama_tsk, new_flags);

out:
	put_yama_filter(old, &rm_old);
	delayed_reclaim_yama_filters(true, rm_old);

	return ret;
}

static int yama_set_op_value(struct task_struct *tsk,
			     unsigned long op, unsigned long value)
{
	int ret;
	struct yama_task *ytask;
	unsigned long flag = 0;

	ret = yama_filter_op_to_flag(op, value, &flag);
	if (ret < 0)
		return ret;

	ytask = give_me_yama_task(tsk);
	if (IS_ERR(ytask))
		return PTR_ERR(ytask);

	ret = yama_set_filter(ytask, op, flag, value);

	put_yama_task(ytask);
	return ret;
}

static int yama_get_op_value(struct task_struct *tsk, unsigned long op)
{
	int ret;
	struct yama_task *ytask;

	ytask = get_yama_task(tsk);
	if (!ytask)
		return -EINVAL;

	ret = yama_task_is_op_set(ytask, op);
	put_yama_task(ytask);

	return ret;
}

int yama_copy_task_filter(struct task_struct *tsk)
{
	int ret = 0;
	struct yama_filter *filter;
	struct yama_task *yparent;
	struct yama_task *ytask = NULL;

	yparent = get_yama_task(current);

	/* Parent does not have a yama filter context, nothing to do */
	if (yparent == NULL)
		return ret;

	filter = get_yama_filter_of_task(yparent);

	ytask = init_yama_task(tsk, filter);
	if (IS_ERR(ytask)) {
		ret = PTR_ERR(ytask);
		put_yama_filter_of_task(yparent, false);
		goto out;
	}

	ret = insert_yama_task(ytask);
	if (ret) {
		kfree(ytask);
		put_yama_filter_of_task(yparent, false);
	}

out:
	put_yama_task(yparent);
	return ret;
}

void yama_free_task_filter(struct task_struct *tsk)
{
	struct yama_task *ytask;

	ytask = lookup_yama_task(tsk);
	if (!ytask)
		return;

	put_yama_task(ytask);
}

int yama_prctl_opts(struct task_struct *tsk, unsigned long arg2,
		    unsigned long arg3, unsigned long arg4)
{
	int ret = -EINVAL;

	switch (arg2) {
	case PR_YAMA_SET_MOD_HARDEN:
		ret = yama_set_op_value(tsk, PR_YAMA_SET_MOD_HARDEN, arg3);
		break;
	case PR_YAMA_GET_MOD_HARDEN:
		ret = yama_get_op_value(tsk, PR_YAMA_GET_MOD_HARDEN);
		break;
	}

	return ret;
}
