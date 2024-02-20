/*****************************************************************************\
 *  job_state.c
 *****************************************************************************
 *  Copyright (C) SchedMD LLC.
 *  Written by Nathan Rini <nate@schedmd.com>
 *
 *  This file is part of Slurm, a resource management program.
 *  For details, see <https://slurm.schedmd.com/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  Slurm is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  Slurm is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Slurm; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#include "src/common/macros.h"

#include "src/slurmctld/slurmctld.h"

#ifndef NDEBUG

#define T(x) { x, XSTRINGIFY(x) }
static const struct {
	uint32_t flag;
	char *string;
} job_flags[] = {
	T(JOB_LAUNCH_FAILED),
	T(JOB_UPDATE_DB),
	T(JOB_REQUEUE),
	T(JOB_REQUEUE_HOLD),
	T(JOB_SPECIAL_EXIT),
	T(JOB_RESIZING),
	T(JOB_CONFIGURING),
	T(JOB_COMPLETING),
	T(JOB_STOPPED),
	T(JOB_RECONFIG_FAIL),
	T(JOB_POWER_UP_NODE),
	T(JOB_REVOKED),
	T(JOB_REQUEUE_FED),
	T(JOB_RESV_DEL_HOLD),
	T(JOB_SIGNALING),
	T(JOB_STAGE_OUT),
};

static void _check_job_state(const uint32_t state)
{
	uint32_t flags;

	if (!(slurm_conf.debug_flags & DEBUG_FLAG_TRACE_JOBS))
		return;

	flags = (state & JOB_STATE_FLAGS);

	xassert((state & JOB_STATE_BASE) < JOB_END);

	for (int i = 0; i < ARRAY_SIZE(job_flags); i++)
		if ((flags & job_flags[i].flag) == job_flags[i].flag)
			flags &= ~(job_flags[i].flag);

	/* catch any bits that are not known flags */
	xassert(!flags);
}

static void _log_job_state_change(const job_record_t *job_ptr,
				  const uint32_t new_state)
{
	char *before_str, *after_str;

	if (!(slurm_conf.debug_flags & DEBUG_FLAG_TRACE_JOBS))
		return;

	before_str = job_state_string_complete(job_ptr->job_state);
	after_str = job_state_string_complete(new_state);

	if (job_ptr->job_state == new_state)
		log_flag(TRACE_JOBS, "%s: [%pJ] no-op change state: %s",
			 __func__, job_ptr, before_str);
	else
		log_flag(TRACE_JOBS, "%s: [%pJ] change state: %s -> %s",
			 __func__, job_ptr, before_str, after_str);

	xfree(before_str);
	xfree(after_str);
}

#else /* NDEBUG */

#define _check_job_state(state) {}
#define _log_job_state_change(job_ptr, new_state) {}

#endif /* NDEBUG */

extern void job_state_set(job_record_t *job_ptr, uint32_t state)
{
	_check_job_state(state);
	_log_job_state_change(job_ptr, state);

	job_ptr->job_state = state;
}

extern void job_state_set_flag(job_record_t *job_ptr, uint32_t flag)
{
	uint32_t job_state;

	xassert(!(flag & JOB_STATE_BASE));
	xassert(flag & JOB_STATE_FLAGS);

	job_state = job_ptr->job_state | flag;
	_check_job_state(job_state);
	_log_job_state_change(job_ptr, job_state);

	job_ptr->job_state = job_state;
}

extern void job_state_unset_flag(job_record_t *job_ptr, uint32_t flag)
{
	uint32_t job_state;

	xassert(!(flag & JOB_STATE_BASE));
	xassert(flag & JOB_STATE_FLAGS);

	job_state = job_ptr->job_state & ~flag;
	_check_job_state(job_state);
	_log_job_state_change(job_ptr, job_state);

	job_ptr->job_state = job_state;
}