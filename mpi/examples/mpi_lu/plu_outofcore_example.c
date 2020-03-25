/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010-2020  Université de Bordeaux, CNRS (LaBRI UMR 5800), Inria
 * Copyright (C) 2013       Thibaut Lambert
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

#include "helper.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <starpu.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "pxlu.h"
//#include "pxlu_kernels.h"

#ifdef STARPU_HAVE_LIBNUMA
#include <numaif.h>
#endif

static unsigned long size = 4096;
static unsigned nblocks = 16;
static unsigned check = 0;
static int p = 1;
static int q = 1;
static unsigned display = 0;
static unsigned no_prio = 0;
static char *path = "./starpu-ooc-files";

#ifdef STARPU_HAVE_LIBNUMA
static unsigned numa = 0;
#endif

static size_t allocated_memory = 0;

static starpu_data_handle_t *dataA_handles;

int get_block_rank(unsigned i, unsigned j);

static void parse_args(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-size") == 0)
		{
			char *argptr;
			size = strtol(argv[++i], &argptr, 10);
		}

		if (strcmp(argv[i], "-nblocks") == 0)
		{
			char *argptr;
			nblocks = strtol(argv[++i], &argptr, 10);
		}

		if (strcmp(argv[i], "-check") == 0)
		{
			check = 1;
		}

		if (strcmp(argv[i], "-display") == 0)
		{
			display = 1;
		}

		if (strcmp(argv[i], "-numa") == 0)
		{
#ifdef STARPU_HAVE_LIBNUMA
			numa = 1;
#else
			fprintf(stderr, "Warning: libnuma is not available\n");
#endif
		}

		if (strcmp(argv[i], "-p") == 0)
		{
			char *argptr;
			p = strtol(argv[++i], &argptr, 10);
		}

		if (strcmp(argv[i], "-q") == 0)
		{
			char *argptr;
			q = strtol(argv[++i], &argptr, 10);
		}

		if (strcmp(argv[i], "-path") == 0)
		{
			path = argv[++i];
		}

		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0)
		{
			fprintf(stderr,"usage: %s [-size n] [-nblocks b] [-check] [-display] [-numa] [-p p] [-q q] [-path PATH]\n", argv[0]);
			fprintf(stderr,"\np * q must be equal to the number of MPI nodes\n");
			exit(0);
		}
	}

#ifdef STARPU_HAVE_VALGRIND_H
	if (RUNNING_ON_VALGRIND)
		size = 16;
#endif
}

unsigned STARPU_PLU(display_flag)(void)
{
	return display;
}

static void fill_block_with_random(TYPE *blockptr, unsigned psize, unsigned pnblocks)
{
	const unsigned block_size = (psize/pnblocks);

	unsigned i, j;
	for (i = 0; i < block_size; i++)
	     for (j = 0; j < block_size; j++)
	     {
		  blockptr[j+i*block_size] = (TYPE)starpu_drand48();
	     }
}

static void create_matrix()
{
	size_t blocksize = (size_t)(size/nblocks)*(size/nblocks)*sizeof(TYPE);
	TYPE *blockptr = malloc(blocksize);
	int fd;
	char *filename;
	unsigned filename_length = strlen(path) + 1 + sizeof(nblocks)*3 + 1 + sizeof(nblocks)*3 + 1;

	filename = malloc(filename_length);

	allocated_memory += nblocks*nblocks*blocksize;

	/* Create the whole matrix on the disk */
	unsigned i,j;
	for (j = 0; j < nblocks; j++)
	{
		for (i = 0; i < nblocks; i++)
		{
			fill_block_with_random(blockptr, size, nblocks);
			if (i == j)
			{
				unsigned tmp;
				for (tmp = 0; tmp < size/nblocks; tmp++)
				{
					blockptr[tmp*((size/nblocks)+1)] += (TYPE)10*nblocks;
				}
			}
			snprintf(filename, filename_length, "%s/%u,%u", path, i, j);
			fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0777);
			if (fd < 0)
			{
				perror("open");
				exit(1);
			}
			if (write(fd, blockptr, blocksize) != (starpu_ssize_t) blocksize)
			{
				fprintf(stderr,"short write");
				exit(1);
			}
			if (close(fd) < 0)
			{
				perror("close");
				exit(1);
			}
		}
	}

	free(blockptr);
	free(filename);
}

static void init_matrix(int rank)
{
	/* Allocate a grid of data handles, not all of them have to be allocated later on */
	dataA_handles = calloc(nblocks*nblocks, sizeof(starpu_data_handle_t));

	size_t blocksize = (size_t)(size/nblocks)*(size/nblocks)*sizeof(TYPE);

	int disk_node = starpu_disk_register(&starpu_disk_unistd_ops, path, STARPU_MAX(1024*1024, size*size*sizeof(TYPE)));
	assert(disk_node >= 0);

	char filename[sizeof(nblocks)*3 + 1 + sizeof(nblocks)*3 + 1];

	/* Allocate all the blocks that belong to this mpi node */
	unsigned i,j;
	for (j = 0; j < nblocks; j++)
	{
		for (i = 0; i < nblocks; i++)
		{
			int block_rank = get_block_rank(i, j);
//			starpu_data_handle_t *handleptr = &dataA_handles[j+nblocks*i];
			starpu_data_handle_t *handleptr = &dataA_handles[j+nblocks*i];

			if (block_rank == rank)
			{
				void *disk_obj;
				snprintf(filename, sizeof(filename), "%u,%u", i, j);
				/* Register it to StarPU */
				disk_obj = starpu_disk_open(disk_node, filename, blocksize);
				if (!disk_obj)
				{
					fprintf(stderr,"could not open %s\n", filename);
					exit(1);
				}
				starpu_matrix_data_register(handleptr, disk_node,
					(uintptr_t) disk_obj, size/nblocks,
					size/nblocks, size/nblocks, sizeof(TYPE));
			}
			else
			{
				starpu_matrix_data_register(handleptr, -1,
					0, size/nblocks,
					size/nblocks, size/nblocks, sizeof(TYPE));
			}
			starpu_data_set_coordinates(*handleptr, 2, j, i);
			starpu_mpi_data_register(*handleptr, j+i*nblocks, block_rank);
		}
	}

	//display_all_blocks(nblocks, size/nblocks);
}

TYPE *STARPU_PLU(get_block)(unsigned i, unsigned j)
{
	(void)i;
	(void)j;
	/* This does not really make sense in out of core */
	assert(0);
}

int get_block_rank(unsigned i, unsigned j)
{
	/* Take a 2D block cyclic distribution */
	/* NB: p (resp. q) is for "direction" i (resp. j) */
	return (j % q) * p + (i % p);
}

starpu_data_handle_t STARPU_PLU(get_block_handle)(unsigned i, unsigned j)
{
	return dataA_handles[j+i*nblocks];
}

int main(int argc, char **argv)
{
	int rank;
	int world_size;
	int ret;
	unsigned i, j;

	starpu_srand48((long int)time(NULL));

	parse_args(argc, argv);

	ret = mkdir(path, 0777);
	if (ret != 0 && errno != EEXIST)
	{
		fprintf(stderr,"%s does not exist\n", path);
		exit(1);
	}

	ret = starpu_mpi_init_conf(&argc, &argv, 1, MPI_COMM_WORLD, NULL);
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_mpi_init_conf");

	starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);
	starpu_mpi_comm_size(MPI_COMM_WORLD, &world_size);

	STARPU_ASSERT(p*q == world_size);

	starpu_cublas_init();

	/*
	 * 	Problem Init
	 */

	if (rank == 0)
		create_matrix();

	starpu_mpi_barrier(MPI_COMM_WORLD);

	init_matrix(rank);

	if (rank == 0)
		fprintf(stderr, "%dMB on disk\n", (int)(allocated_memory/(1024*1024)));

	TYPE *a_r = NULL;
//	STARPU_PLU(display_data_content)(a_r, size);

	if (check)
	{
		TYPE *x, *y;

		x = calloc(size, sizeof(TYPE));
		STARPU_ASSERT(x);

		y = calloc(size, sizeof(TYPE));
		STARPU_ASSERT(y);

		if (rank == 0)
		{
			unsigned ind;
			for (ind = 0; ind < size; ind++)
				x[ind] = (TYPE)starpu_drand48();
		}

		a_r = STARPU_PLU(reconstruct_matrix)(size, nblocks);

		if (rank == 0)
			STARPU_PLU(display_data_content)(a_r, size);

//		STARPU_PLU(compute_ax)(size, x, y, nblocks, rank);

		free(x);
		free(y);
	}

	double timing = STARPU_PLU(plu_main)(nblocks, rank, world_size, no_prio);

	/*
	 * 	Report performance
	 */

	if (rank == 0)
	{
		fprintf(stderr, "Computation took: %f ms\n", timing/1000);

		unsigned n = size;
		double flop = (2.0f*n*n*n)/3.0f;
		fprintf(stderr, "Synthetic GFlops : %2.2f\n", (flop/timing/1000.0f));
	}

	/*
	 *	Test Result Correctness
	 */

	if (check)
	{
		/*
		 *	Compute || A - LU ||
		 */

		STARPU_PLU(compute_lu_matrix)(size, nblocks, a_r);

#if 0
		/*
		 *	Compute || Ax - LUx ||
		 */

		unsigned ind;

		y2 = calloc(size, sizeof(TYPE));
		STARPU_ASSERT(y);

		if (rank == 0)
		{
			for (ind = 0; ind < size; ind++)
			{
				y2[ind] = (TYPE)0.0;
			}
		}

		STARPU_PLU(compute_lux)(size, x, y2, nblocks, rank);

		/* Compute y2 = y2 - y */
		CPU_AXPY(size, -1.0, y, 1, y2, 1);

		TYPE err = CPU_ASUM(size, y2, 1);
		int max = CPU_IAMAX(size, y2, 1);

		fprintf(stderr, "(A - LU)X Avg error : %e\n", err/(size*size));
		fprintf(stderr, "(A - LU)X Max error : %e\n", y2[max]);
#endif
	}

	/*
	 * 	Termination
	 */
	for (j = 0; j < nblocks; j++)
	{
		for (i = 0; i < nblocks; i++)
		{
			starpu_data_unregister(dataA_handles[j+nblocks*i]);
		}
	}

	starpu_cublas_shutdown();
	starpu_mpi_shutdown();

	return 0;
}
