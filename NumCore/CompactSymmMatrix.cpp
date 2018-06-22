#include "stdafx.h"
#include "CompactSymmMatrix.h"

#ifdef PARDISO
#include "mkl_spblas.h"
#endif

//-----------------------------------------------------------------------------
//! constructor
CompactSymmMatrix::CompactSymmMatrix(int offset) : CompactMatrix(offset) 
{
	
}

//-----------------------------------------------------------------------------
void CompactSymmMatrix::mult_vector(double* x, double* r)
{
	// get row count
	int N = Rows();
	int M = Columns();

	// zero result vector
	for (int j = 0; j<N; ++j) r[j] = 0.0;

#ifdef PARDISO
	char tr = 'u';
	double* a = Values();
	int* ia = Pointers();
	int* ja = Indices();
	mkl_dcsrsymv(&tr, &N, a, ia, ja, x, r);
#else
	// loop over all columns
	for (int j = 0; j<M; ++j)
	{
		double* pv = m_pd + m_ppointers[j] - m_offset;
		int* pi = m_pindices + m_ppointers[j] - m_offset;
		int n = m_ppointers[j + 1] - m_ppointers[j];

		// add off-diagonal elements
		for (int i = 1; i<n - 7; i += 8)
		{
			// add lower triangular element
			r[pi[i    ] - m_offset] += pv[i    ] * x[j];
			r[pi[i + 1] - m_offset] += pv[i + 1] * x[j];
			r[pi[i + 2] - m_offset] += pv[i + 2] * x[j];
			r[pi[i + 3] - m_offset] += pv[i + 3] * x[j];
			r[pi[i + 4] - m_offset] += pv[i + 4] * x[j];
			r[pi[i + 5] - m_offset] += pv[i + 5] * x[j];
			r[pi[i + 6] - m_offset] += pv[i + 6] * x[j];
			r[pi[i + 7] - m_offset] += pv[i + 7] * x[j];
		}
		for (int i = 0; i<(n - 1) % 8; ++i)
			r[pi[n - 1 - i] - m_offset] += pv[n - 1 - i] * x[j];

		// add diagonal element
		double rj = pv[0] * x[j];

		// add upper-triangular elements
		for (int i = 1; i<n - 7; i += 8)
		{
			// add upper triangular element
			rj += pv[i    ] * x[pi[i    ] - m_offset];
			rj += pv[i + 1] * x[pi[i + 1] - m_offset];
			rj += pv[i + 2] * x[pi[i + 2] - m_offset];
			rj += pv[i + 3] * x[pi[i + 3] - m_offset];
			rj += pv[i + 4] * x[pi[i + 4] - m_offset];
			rj += pv[i + 5] * x[pi[i + 5] - m_offset];
			rj += pv[i + 6] * x[pi[i + 6] - m_offset];
			rj += pv[i + 7] * x[pi[i + 7] - m_offset];
		}
		for (int i = 0; i<(n - 1) % 8; ++i)
			rj += pv[n - 1 - i] * x[pi[n - 1 - i] - m_offset];

		r[j] += rj;
	}
#endif
}

//-----------------------------------------------------------------------------
void CompactSymmMatrix::Create(SparseMatrixProfile& mp)
{
	// TODO: we should probably enforce that the matrix is square
	int nr = mp.Rows();
	int nc = mp.Columns();
	assert(nr==nc);

	// allocate pointers to column offsets
	int* pointers = new int[nc + 1];
	for (int i = 0; i <= nc; ++i) pointers[i] = 0;

	int nsize = 0;
	for (int i = 0; i<nc; ++i)
	{
		vector<int>& a = mp.Column(i);
		int n = (int)a.size();
		for (int j = 0; j<n; j += 2)
		{
			int a0 = a[j];
			int a1 = a[j + 1];

			// only grab lower-triangular
			if (a1 >= i)
			{
				if (a0 < i) a0 = i;
				int asize = a1 - a0 + 1;
				nsize += asize;
				pointers[i] += asize;
			}
		}
	}

	// allocate indices which store row index for each matrix element
	int* pindices = new int[nsize];
	int m = 0;
	for (int i = 0; i <= nc; ++i)
	{
		int n = pointers[i];
		pointers[i] = m;
		m += n;
	}

	for (int i = 0; i<nc; ++i)
	{
		vector<int>& a = mp.Column(i);
		int n = (int)a.size();
		int nval = 0;
		for (int j = 0; j<n; j += 2)
		{
			int a0 = a[j];
			int a1 = a[j + 1];

			// only grab lower-triangular
			if (a1 >= i)
			{
				if (a0 < i) a0 = i;
				for (int k = a0; k <= a1; ++k)
				{
					pindices[pointers[i] + nval] = k;
					++nval;
				}
			}
		}
	}

	// offset the indicies for fortran arrays
	if (Offset())
	{
		for (int i = 0; i <= nc; ++i) pointers[i]++;
		for (int i = 0; i<nsize; ++i) pindices[i]++;
	}

	// create the values array
	double* pvalues = new double[nsize];

	// create the stiffness matrix
	CompactMatrix::alloc(nr, nc, nsize, pvalues, pindices, pointers);
}

//-----------------------------------------------------------------------------
// this sort function is defined in qsort.cpp
void qsort(int n, int* arr, int* indx);

//-----------------------------------------------------------------------------
//! This function assembles the local stiffness matrix
//! into the global stiffness matrix which is in compact column storage
//!
void CompactSymmMatrix::Assemble(matrix& ke, vector<int>& LM)
{
	// get the number of degrees of freedom
	const int N = ke.rows();

	// find the permutation array that sorts LM in ascending order
	// we can use this to speed up the row search (i.e. loop over n below)
	static vector<int> P; P.resize(N);
	qsort(N, &LM[0], &P[0]);

	// get the data pointers 
	int* indices = Indices();
	int* pointers = Pointers();
	double* pd = Values();
	int offset = Offset();

	// find the starting index
	int N0 = 0;
	while ((N0<N) && (LM[P[N0]]<0)) ++N0;

	// assemble element stiffness
	for (int m = N0; m<N; ++m)
	{
		int j = P[m];
		int J = LM[j];
		int n = 0;
		double* pm = pd + (pointers[J] - offset);
		int* pi = indices + (pointers[J] - offset);
		int l = pointers[J + 1] - pointers[J];
		int M0 = m;
		while ((M0>N0) && (LM[P[M0 - 1]] == J)) M0--;
		for (int k = M0; k<N; ++k)
		{
			int i = P[k];
			int I = LM[i] + offset;
			for (; n<l; ++n)
				if (pi[n] == I)
				{
					pm[n] += ke[i][j];
					break;
				}
		}
	}
}


//-----------------------------------------------------------------------------
void CompactSymmMatrix::Assemble(matrix& ke, vector<int>& LMi, vector<int>& LMj)
{
	int i, j, I, J;

	const int N = ke.rows();
	const int M = ke.columns();

	int* indices = Indices();
	int* pointers = Pointers();
	double* pd = Values();

	int *pi, l, n;

	for (i = 0; i<N; ++i)
	{
		I = LMi[i];

		for (j = 0; j<M; ++j)
		{
			J = LMj[j];

			// only add values to lower-diagonal part of stiffness matrix
			if ((I >= J) && (J >= 0))
			{
				pi = indices + pointers[J];
				l = pointers[J + 1] - pointers[J];
				for (n = 0; n<l; ++n) if (pi[n] == I)
				{
					pd[pointers[J] + n] += ke[i][j];
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
//! add a matrix item
void CompactSymmMatrix::add(int i, int j, double v)
{
	// only add to lower triangular part
	// since FEBio only works with the upper triangular part
	// we have to swap the indices
	i ^= j; j ^= i; i ^= j;

	if (j <= i)
	{
		double* pd = m_pd + (m_ppointers[j] - m_offset);
		int* pi = m_pindices + m_ppointers[j];
		pi -= m_offset;
		i += m_offset;
		int n1 = m_ppointers[j + 1] - m_ppointers[j] - 1;
		int n0 = 0;
		int n = n1 / 2;
		do
		{
			int m = pi[n];
			if (m == i)
			{
				pd[n] += v;
				return;
			}
			else if (m < i)
			{
				n0 = n;
				n = (n0 + n1 + 1) >> 1;
			}
			else
			{
				n1 = n;
				n = (n0 + n1) >> 1;
			}
		} while (n0 != n1);

		assert(false);
	}
}

//-----------------------------------------------------------------------------
//! check fo a matrix item
bool CompactSymmMatrix::check(int i, int j)
{
	// only add to lower triangular part
	if (j <= i)
	{
		int* pi = m_pindices + m_ppointers[j];
		pi -= m_offset;
		int l = m_ppointers[j + 1] - m_ppointers[j];
		for (int n = 0; n<l; ++n)
			if (pi[n] == i + m_offset)
			{
				return true;
			}
	}

	return false;
}


//-----------------------------------------------------------------------------
//! set matrix item
void CompactSymmMatrix::set(int i, int j, double v)
{
	if (j <= i)
	{
		int* pi = m_pindices + m_ppointers[j];
		pi -= m_offset;
		int l = m_ppointers[j + 1] - m_ppointers[j];
		for (int n = 0; n<l; ++n)
			if (pi[n] == i + m_offset)
			{
				int k = m_ppointers[j] + n;
				k -= m_offset;
				m_pd[k] = v;
				return;
			}

		assert(false);
	}
}

//-----------------------------------------------------------------------------
//! get a matrix item
double CompactSymmMatrix::get(int i, int j)
{
	// only the lower-triangular part is stored, so swap indices if necessary
	if (i<j) { i ^= j; j ^= i; i ^= j; }

	int *pi = m_pindices + m_ppointers[j], k;
	pi -= m_offset;
	int l = m_ppointers[j + 1] - m_ppointers[j];
	for (int n = 0; n<l; ++n)
		if (pi[n] == i + m_offset)
		{
			k = m_ppointers[j] + n;
			k -= m_offset;
			return m_pd[k];
		}
	return 0;
}