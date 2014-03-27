/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2014  Inria
 *
 * StarPU is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * StarPU is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */

#include <starpu.h>
#include <util/openmp_runtime_support.h>

#ifdef STARPU_OPENMP
#define __not_implemented__ do { fprintf (stderr, "omp lib function %s not implemented\n", __func__); abort(); } while (0)

static double _starpu_omp_clock_ref = 0.0; /* clock reference for starpu_omp_get_wtick */

static struct starpu_omp_global_icvs _global_icvs;
static struct starpu_omp_initial_icv_values _initial_icv_values =
{
	.dyn_var = 0,
	.nest_var = 0,
	.nthreads_var = NULL,
	.run_sched_var = 0,
	.def_sched_var = 0,
	.bind_var = NULL,
	.stacksize_var = 0,
	.wait_policy_var = 0,
	.max_active_levels_var = 0,
	.active_levels_var = 0,
	.levels_var = 0,
	.place_partition_var = 0,
	.cancel_var = 0,
	.default_device_var = 0
};

struct starpu_omp_global_icvs *starpu_omp_global_icvs = NULL;
struct starpu_omp_initial_icv_values *starpu_omp_initial_icv_values = NULL;

static void read_int_var(const char *var, int *dest)
{
	const char *env = getenv(var);
	if (env) {
		int v = (int)strtol(env, NULL, 16);
		if (errno != 0) {
			fprintf(stderr, "Warning: could not parse environment variable %s, strtol failed with error %s\n", var, strerror(errno));
		} else {
			*dest = v;
		}
	}
}

/*
 * Entry point to be called by the OpenMP runtime constructor
 */
int starpu_omp_init(void)
{
	int ret;

	read_int_var("OMP_DYNAMIC", &_initial_icv_values.dyn_var);
	read_int_var("OMP_NESTED", &_initial_icv_values.nest_var);
	/* TODO: OMP_NUM_THREADS */
	read_int_var("OMP_SCHEDULE", &_initial_icv_values.run_sched_var);
	/* TODO: OMP_PROC_BIND */
	read_int_var("OMP_STACKSIZE", &_initial_icv_values.stacksize_var);
	read_int_var("OMP_WAIT_POLICY", &_initial_icv_values.wait_policy_var);
	read_int_var("OMP_THREAD_LIMIT", &_initial_icv_values.thread_limit_var);
	read_int_var("OMP_MAX_ACTIVE_LEVELS", &_initial_icv_values.max_active_levels_var);
	read_int_var("OMP_PLACES", &_initial_icv_values.place_partition_var);
	read_int_var("OMP_CANCELLATION", &_initial_icv_values.cancel_var);
	read_int_var("OMP_DEFAULT_DEVICE", &_initial_icv_values.default_device_var);
	starpu_omp_initial_icv_values = &_initial_icv_values;

	_global_icvs.cancel_var = starpu_omp_initial_icv_values->cancel_var;
	starpu_omp_global_icvs = &_global_icvs;

	ret = starpu_init(0);
	if(ret < 0)
		return ret;

	/* init clock reference for starpu_omp_get_wtick */
	_starpu_omp_clock_ref = starpu_timing_now();

	return 0;
}

void starpu_omp_shutdown(void)
{
	starpu_shutdown();
}

void starpu_omp_set_num_threads(int threads)
{
	(void) threads;
	__not_implemented__;
}

int starpu_omp_get_num_threads()
{
	return starpu_cpu_worker_get_count();
}

int starpu_omp_get_thread_num()
{
	int tid = starpu_worker_get_id();
	/* TODO: handle master thread case */
	if (tid < 0)
	{
		fprintf(stderr, "starpu_omp_get_thread_num: no worker associated to this thread\n");
		abort();
	}
	return tid;
}

int starpu_omp_get_max_threads()
{
	/* arbitrary limit */
	return starpu_cpu_worker_get_count();
}

int starpu_omp_get_num_procs (void)
{
	/* starpu_cpu_worker_get_count defined as topology.ncpus */
	return starpu_cpu_worker_get_count();
}

int starpu_omp_in_parallel (void)
{
	__not_implemented__;
}

void starpu_omp_set_dynamic (int dynamic_threads)
{
	(void) dynamic_threads;
	/* TODO: dynamic adjustment of the number of threads is not supported for now */
}

int starpu_omp_get_dynamic (void)
{
	/* TODO: dynamic adjustment of the number of threads is not supported for now 
	 * return false as required */
	return 0;
}

void starpu_omp_set_nested (int nested)
{
	(void) nested;
	/* TODO: nested parallelism not supported for now */
}

int starpu_omp_get_nested (void)
{
	/* TODO: nested parallelism not supported for now
	 * return false as required */
	return 0;
}

int starpu_omp_get_cancellation(void)
{
	/* TODO: cancellation not supported for now
	 * return false as required */
	return 0;
}

void starpu_omp_set_schedule (starpu_omp_sched_t kind, int modifier)
{
	(void) kind;
	(void) modifier;
	/* TODO: no starpu_omp scheduler scheme implemented for now */
	__not_implemented__;
	assert(kind >= 1 && kind <=4);
}

void starpu_omp_get_schedule (starpu_omp_sched_t *kind, int *modifier)
{
	(void) kind;
	(void) modifier;
	/* TODO: no starpu_omp scheduler scheme implemented for now */
	__not_implemented__;
}

int starpu_omp_get_thread_limit (void)
{
	/* arbitrary limit */
	return 1024;
}

void starpu_omp_set_max_active_levels (int max_levels)
{
	(void) max_levels;
	/* TODO: nested parallelism not supported for now */
}

int starpu_omp_get_max_active_levels (void)
{
	/* TODO: nested parallelism not supported for now
	 * assume a single level */
	return 1;
}

int starpu_omp_get_level (void)
{
	/* TODO: nested parallelism not supported for now
	 * assume a single level */
	return 1;
}

int starpu_omp_get_ancestor_thread_num (int level)
{
	if (level == 0) {
		return 0; /* spec required answer */
	}

	if (level == starpu_omp_get_level()) {
		return starpu_omp_get_thread_num(); /* spec required answer */
	}

	/* TODO: nested parallelism not supported for now
	 * assume ancestor is thread number '0' */
	return 0;
}

int starpu_omp_get_team_size (int level)
{
	if (level == 0) {
		return 1; /* spec required answer */
	}

	if (level == starpu_omp_get_level()) {
		return starpu_omp_get_num_threads(); /* spec required answer */
	}

	/* TODO: nested parallelism not supported for now
	 * assume the team size to be the number of cpu workers */
	return starpu_cpu_worker_get_count();
}

int starpu_omp_get_active_level (void)
{
	/* TODO: nested parallelism not supported for now
	 * assume a single active level */
	return 1;
}

int starpu_omp_in_final(void)
{
	/* TODO: final not supported for now
	 * assume not in final */
	return 0;
}

starpu_omp_proc_bind_t starpu_omp_get_proc_bind(void)
{
	/* TODO: proc_bind not supported for now
	 * assumre false */
	return starpu_omp_proc_bind_false;
}

void starpu_omp_set_default_device(int device_num)
{
	(void) device_num;
	/* TODO: set_default_device not supported for now */
}

int starpu_omp_get_default_device(void)
{
	/* TODO: set_default_device not supported for now
	 * assume device 0 as default */
	return 0;
}

int starpu_omp_get_num_devices(void)
{
	/* TODO: get_num_devices not supported for now
	 * assume 1 device */
	return 1;
}

int starpu_omp_get_num_teams(void)
{
	/* TODO: num_teams not supported for now
	 * assume 1 team */
	return 1;
}

int starpu_omp_get_team_num(void)
{
	/* TODO: team_num not supported for now
	 * assume team_num 0 */
	return 0;
}

int starpu_omp_is_initial_device(void)
{
	/* TODO: is_initial_device not supported for now
	 * assume host device */
	return 1;
}


void starpu_omp_init_lock (starpu_omp_lock_t *lock)
{
	(void) lock;
	__not_implemented__;
}

void starpu_omp_destroy_lock (starpu_omp_lock_t *lock)
{
	(void) lock;
	__not_implemented__;
}

void starpu_omp_set_lock (starpu_omp_lock_t *lock)
{
	(void) lock;
	__not_implemented__;
}

void starpu_omp_unset_lock (starpu_omp_lock_t *lock)
{
	(void) lock;
	__not_implemented__;
}

int starpu_omp_test_lock (starpu_omp_lock_t *lock)
{
	(void) lock;
	__not_implemented__;
}

void starpu_omp_init_nest_lock (starpu_omp_nest_lock_t *lock)
{
	(void) lock;
	__not_implemented__;
}

void starpu_omp_destroy_nest_lock (starpu_omp_nest_lock_t *lock)
{
	(void) lock;
	__not_implemented__;
}

void starpu_omp_set_nest_lock (starpu_omp_nest_lock_t *lock)
{
	(void) lock;
	__not_implemented__;
}

void starpu_omp_unset_nest_lock (starpu_omp_nest_lock_t *lock)
{
	(void) lock;
	__not_implemented__;
}

int starpu_omp_test_nest_lock (starpu_omp_nest_lock_t *lock)
{
	(void) lock;
	__not_implemented__;
}

double starpu_omp_get_wtime (void)
{
	return starpu_timing_now() - _starpu_omp_clock_ref;
}

double starpu_omp_get_wtick (void)
{
	/* arbitrary precision value */
	return 1e-6;
}
#endif /* STARPU_OPENMP */
