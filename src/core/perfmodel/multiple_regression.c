/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2009, 2010, 2011, 2015  Université de Bordeaux
 * Copyright (C) 2010, 2011  CNRS
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

/* Code for computing multiple linear regression */

#include <core/perfmodel/multiple_regression.h>

#ifdef TESTGSL
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_multifit.h>
#endif //TESTGSL

#ifdef DGELS
typedef long int integer;
typedef double doublereal;

int dgels_(char *trans, integer *m, integer *n, integer *nrhs, doublereal *a, integer *lda, doublereal *b, integer *ldb, doublereal *work, integer *lwork, integer *info);
#endif //DGELS

static long count_file_lines(FILE *f)
{
	int ch, lines=0;
	while(!feof(f))
	{
		ch = fgetc(f);
	    if(ch == '\n')
	    {
		  lines++;
		}
    }
	rewind(f);

	return lines;
}

static void dump_multiple_regression_list(double *mpar, double *my, int start, unsigned nparameters, struct starpu_perfmodel_history_list *list_history)
{
	struct starpu_perfmodel_history_list *ptr = list_history;
	int i = start;
	while (ptr)
	{
		my[i] = ptr->entry->duration;
		for(int j=0; j<nparameters; j++)
			mpar[i*nparameters+j] = ptr->entry->parameters[j];
		ptr = ptr->next;
		i++;
	}

}

static void load_old_calibration(double *mx, double *my, unsigned nparameters, FILE *f)
{
	char buffer[1024];
	char *record,*line;
	int i=0,j=0;

	line=fgets(buffer,sizeof(buffer),f);//skipping first line
	while((line=fgets(buffer,sizeof(buffer),f))!=NULL)
	{
		record = strtok(line,",");
		my[i] = atof(record);
		record = strtok(NULL,",");
		j=0;
		while(record != NULL)
		{
			mx[i*nparameters+j] = atof(record) ;
			++j;
			record = strtok(NULL,",");
		}
		++i ;
	}
}

static long find_long_list_size(struct starpu_perfmodel_history_list *list_history)
{
	long cnt = 0;

	struct starpu_perfmodel_history_list *ptr = list_history;
	while (ptr)
	{
		cnt++;
		ptr = ptr->next;
	}

	return cnt;
}

#ifdef TESTGSL
void gsl_multiple_reg_coeff(double *mpar, double *my, long n, unsigned ncoeff, unsigned nparameters, double *coeff, unsigned **combinations)
{
	double coefficient;
	gsl_matrix *X = gsl_matrix_calloc(n, ncoeff);
	gsl_vector *Y = gsl_vector_alloc(n);
	gsl_vector *beta = gsl_vector_alloc(ncoeff);

	for (int i = 0; i < n; i++) {
		gsl_vector_set(Y, i, my[i]);
		gsl_matrix_set(X, i, 0, 1.);
		for (int j = 1; j < ncoeff; j++)
		{
			coefficient = 1.;
			for(int k=0; k < nparameters; k++)
			{
				coefficient *= pow(mpar[i*nparameters+k],combinations[j-1][k]);
			}
			gsl_matrix_set(X, i, j, coefficient);
		}
	}

	double chisq;
	gsl_matrix *cov = gsl_matrix_alloc(ncoeff, ncoeff);
	gsl_multifit_linear_workspace * wspc = gsl_multifit_linear_alloc(n, ncoeff);
	gsl_multifit_linear(X, Y, beta, cov, &chisq, wspc);

	for(int i=0; i<ncoeff; i++)
		coeff[i] = gsl_vector_get(beta, i);

	gsl_matrix_free(X);
	gsl_matrix_free(cov);
	gsl_vector_free(Y);
	gsl_vector_free(beta);
	gsl_multifit_linear_free(wspc);
}
#endif //TESTGSL

#ifdef DGELS
int dgels_multiple_reg_coeff(double *mpar, double *my, long nn, unsigned ncoeff, unsigned nparameters, double *coeff, unsigned **combinations)
{	
 /*  Arguments */
/*  ========= */

/*  TRANS   (input) CHARACTER*1 */
/*          = 'N': the linear system involves A; */
/*          = 'T': the linear system involves A**T. */

/*  M       (input) INTEGER */
/*          The number of rows of the matrix A.  M >= 0. */

/*  N       (input) INTEGER */
/*          The number of columns of the matrix A.  N >= 0. */

/*  NRHS    (input) INTEGER */
/*          The number of right hand sides, i.e., the number of */
/*          columns of the matrices B and X. NRHS >=0. */

/*  A       (input/output) DOUBLE PRECISION array, dimension (LDA,N) */
/*          On entry, the M-by-N matrix A. */
/*          On exit, */
/*            if M >= N, A is overwritten by details of its QR */
/*                       factorization as returned by DGEQRF; */
/*            if M <  N, A is overwritten by details of its LQ */
/*                       factorization as returned by DGELQF. */

/*  LDA     (input) INTEGER */
/*          The leading dimension of the array A.  LDA >= max(1,M). */

/*  B       (input/output) DOUBLE PRECISION array, dimension (LDB,NRHS) */
/*          On entry, the matrix B of right hand side vectors, stored */
/*          columnwise; B is M-by-NRHS if TRANS = 'N', or N-by-NRHS */
/*          if TRANS = 'T'. */
/*          On exit, if INFO = 0, B is overwritten by the solution */
/*          vectors, stored columnwise: */
/*          if TRANS = 'N' and m >= n, rows 1 to n of B contain the least */
/*          squares solution vectors; the residual sum of squares for the */
/*          solution in each column is given by the sum of squares of */
/*          elements N+1 to M in that column; */
/*          if TRANS = 'N' and m < n, rows 1 to N of B contain the */
/*          minimum norm solution vectors; */
/*          if TRANS = 'T' and m >= n, rows 1 to M of B contain the */
/*          minimum norm solution vectors; */
/*          if TRANS = 'T' and m < n, rows 1 to M of B contain the */
/*          least squares solution vectors; the residual sum of squares */
/*          for the solution in each column is given by the sum of */
/*          squares of elements M+1 to N in that column. */

/*  LDB     (input) INTEGER */
/*          The leading dimension of the array B. LDB >= MAX(1,M,N). */

/*  WORK    (workspace/output) DOUBLE PRECISION array, dimension (MAX(1,LWORK)) */
/*          On exit, if INFO = 0, WORK(1) returns the optimal LWORK. */

/*  LWORK   (input) INTEGER */
/*          The dimension of the array WORK. */
/*          LWORK >= max( 1, MN + max( MN, NRHS ) ). */
/*          For optimal performance, */
/*          LWORK >= max( 1, MN + max( MN, NRHS )*NB ). */
/*          where MN = min(M,N) and NB is the optimum block size. */

/*          If LWORK = -1, then a workspace query is assumed; the routine */
/*          only calculates the optimal size of the WORK array, returns */
/*          this value as the first entry of the WORK array, and no error */
/*          message related to LWORK is issued by XERBLA. */

/*  INFO    (output) INTEGER */
/*          = 0:  successful exit */
/*          < 0:  if INFO = -i, the i-th argument had an illegal value */
/*          > 0:  if INFO =  i, the i-th diagonal element of the */
/*                triangular factor of A is zero, so that A does not have */
/*                full rank; the least squares solution could not be */
/*                computed. */

/*  ===================================================================== */

	if(nn <= ncoeff)
	{
		_STARPU_DEBUG("ERROR: This function is not intended for the use when number of parameters is larger than the number of observations. Check how your matrices A and B were allocated or simply add more benchmarks.\n");
		return 1;
	}
	
	char trans = 'N';
	integer m = nn;
	integer n = ncoeff;
	integer nrhs = 1; // number of columns of B and X (wich are vectors therefore nrhs=1)
	doublereal *X = malloc(sizeof(double)*n*m); // (/!\ modified at the output) contain the model and the different values of pararmters
	doublereal *Y = malloc(sizeof(double)*m);

	double coefficient;
	for (int i=0; i < m; i++)
	{
		Y[i] = my[i];
		X[i*n] = 1.;
		for (int j=1; j < n; j++)
		{
			coefficient = 1.;
			for(int k=0; k < nparameters; k++)
			{
				coefficient *= pow(mpar[i*nparameters+k],combinations[j-1][k]);
			}			
			X[i*n+j] = coefficient;
		}
	}

	integer lda = m; 
	integer ldb = m; //
	integer info = 0;

	integer lwork = n*2;
	doublereal *work = malloc(sizeof(double)*lwork); // (output)

	/* // Running LAPACK dgels_ */
	dgels_(&trans, &m, &n, &nrhs, X, &lda, Y, &ldb, work, &lwork, &info);

	/* Check for the full rank */
	if( info != 0 )
	{
		_STARPU_DEBUG("Problems with DGELS; info=%ld\n", info);
		return 1;
	}

	/* Copy computed coefficients */
	for(int i=0; i<ncoeff; i++)
		coeff[i] = Y[i];

	free(X);
	free(Y);
	free(work);
	
	return 0;
}
#endif //DGELS


/*
   Validating the accuracy of the coefficients.
   For the the validation is extremely basic, but it should be improved.
 */
int invalidate(double *coeff, unsigned ncoeff)
{
	if (coeff[0] < 0)
	{
		_STARPU_DEBUG("Constant in computed by least square method is negative (%f)\n", coeff[0]);
		return 1;
	}
		
	for(int i=1; i<ncoeff; i++)
	{
		if(coeff[i] < 1E-10)
		{
			_STARPU_DEBUG("Coefficient computed by  least square method is too small (%f)\n", coeff[i]);
			return 1;
		}
	}

	return 0;
}
	
int _starpu_multiple_regression(struct starpu_perfmodel_history_list *ptr, double *coeff, unsigned ncoeff, unsigned nparameters, unsigned **combinations, const char *codelet_name)
{
	// Computing number of rows
	long n=find_long_list_size(ptr);
	STARPU_ASSERT(n);
	
        // Reading old calibrations if necessary
	FILE *f;
	char filepath[50];
	snprintf(filepath, 50, "/tmp/%s.out", codelet_name);
	long old_lines=0;
	int calibrate = starpu_get_env_number("STARPU_CALIBRATE");	
	if (calibrate==1)
	{
		f = fopen(filepath, "a+");
		STARPU_ASSERT_MSG(f, "Could not save performance model %s\n", filepath);
		
		old_lines=count_file_lines(f);
		STARPU_ASSERT(old_lines);

		n+=old_lines;
	}

	// Allocating X and Y matrices
	double *mpar = (double *) malloc(nparameters*n*sizeof(double));
	STARPU_ASSERT(mpar);
	double *my = (double *) malloc(n*sizeof(double));
	STARPU_ASSERT(my);

	// Loading old calibration
	if (calibrate==1)
		load_old_calibration(mpar, my, nparameters, f);

	// Filling X and Y matrices with measured values
	dump_multiple_regression_list(mpar, my, old_lines, nparameters, ptr);
	
	// Computing coefficients using multiple linear regression
#ifdef DGELS
	if(dgels_multiple_reg_coeff(mpar, my, n, ncoeff, nparameters, coeff, combinations))
		return 1;
#elif TESTGSL
	gsl_multiple_reg_coeff(mpar, my, n, ncoeff, nparameters, coeff, combinations);	
#else
	_STARPU_DEBUG("No function to compute coefficients of multiple linear regression");
	return 1;
#endif

	// Validate the accuracy of the model
	if(invalidate(coeff, ncoeff))
		return 1;
	
	// Preparing new output calibration file
	if (calibrate==2)
	{
		f = fopen(filepath, "w+");
		STARPU_ASSERT_MSG(f, "Could not save performance model %s\n", filepath);
		fprintf(f, "Duration");
		for(int k=0; k < nparameters; k++)
		{
			fprintf(f, ", P%d", k);
		}
	}
	
	// Writing parameters to calibration file
	if (calibrate==1 || calibrate==2)
	{
		for(int i=old_lines; i<n; i++)
		{
			fprintf(f, "\n%f", my[i]);
			for(int j=0; j<nparameters;j++)
				fprintf(f, ", %f", mpar[i*nparameters+j]);
		}
		fclose(f);
	}

	// Cleanup
	free(mpar);
	free(my);

	return 0;
}
