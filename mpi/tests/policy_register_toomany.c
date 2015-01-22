/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2015  Centre National de la Recherche Scientifique
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

#include <starpu_mpi.h>
#include <starpu_mpi_select_node.h>
#include "helper.h"

int starpu_mpi_select_node_my_policy(int me, int nb_nodes, struct starpu_data_descr *descr, int nb_data)
{
	(void) me;
	(void) nb_nodes;
	(void) descr;
	(void) nb_data;
	return 0;
}

int main(int argc, char **argv)
{
	int ret;
	int i, policy;

	ret = starpu_init(NULL);
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");
	ret = starpu_mpi_init(&argc, &argv, 1);
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_mpi_init");

	for(i=0 ; i<_STARPU_MPI_NODE_SELECTION_MAX_POLICY+1 ; i++)
	{
		policy = starpu_mpi_node_selection_register_policy(starpu_mpi_select_node_my_policy);
		FPRINTF_MPI(stderr, "New policy %d\n", policy);
	}

	starpu_mpi_shutdown();
	starpu_shutdown();

	return 0;
}
