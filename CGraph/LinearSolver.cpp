#include "LinearSolver.h"
#include <cmath>
using namespace std;

LinearSolver::LinearSolver(void)
{
}

LinearSolver::~LinearSolver(void)
{
}

int LinearSolver::eliminate_zero(int n, float at[], float b[])
{
	int i, j, k, maxdiarow;
	float maxdia, temp;
	for (i = 0; i<n; i++)
	{
		//downward scan
		maxdia = fabs(at[i*n+i]);
		maxdiarow = i;
		for (j = i; j<n; j++)
		{
			if (fabs(at[j*n+i]) > maxdia)
			{
				maxdiarow = j;
			}
		}

		if (at[maxdiarow*n+i]<ZERO)
		{
			//upward scan
			maxdia = fabs(at[i*n+i]);
			maxdiarow = i;
			for (j = i; j >= 0; j--)
			{
				if (fabs(at[j*n+i]) > maxdia)
				{
					maxdiarow = j;
				}
			}

			if (at[maxdiarow*n+i]<ZERO)//all are the 0
			{
				return -1;
			}
			else
			{
				for (k = 0; k<n; k++)
				{
					at[i*n+k] = at[i*n+k] + at[maxdiarow*n+k];//add the row to this row
				}
				b[i] = b[i] + b[maxdiarow];
			}
		}
		else
		{
			for (k = 0; k<n; k++)
			{
				temp = at[i*n+k];
				at[i*n + k] = at[maxdiarow*n + k];
				at[maxdiarow*n+k] = temp;
			}
			temp = b[i];
			b[i] = b[maxdiarow];
			b[maxdiarow] = temp;
		}
	}
	return 0;
}

int LinearSolver::Gauss_eliminate(int n,float a[], float b[],float x[])
{
	//eliminate the trace that is 0
	int temp = eliminate_zero(n, a, b);
	int i, j, k;

	//eliminate with Gauss
	for (i = 0; i < n - 1; i++)
	{
		if (a[i*n+i] == 0)
		{
			return -1;
		}

		for (j = i + 1; j < n; j++)
		{
			float div = -1 * a[j*n + i] / a[i*n + i];
			for (k = 0; k < n; k++)
			{
				a[j*n + k] += (a[i*n + k] * div);
			}
			b[j] += (b[i] * div);
		}
	}

	//solve the x
	for (i = n - 1; i >= 0; i--)
	{
		if (a[i*n + i] == 0)
		{
			x[i] = 0;
		}
		else
		{
			float sum = 0;
			for (j = n - 1; j > i; j--)
			{
				sum += a[i*n+j] * x[j];
			}
			x[i] = (b[i] - sum) / a[i*n + i];
		}
	}

	return 0;
}

int LinearSolver::Gauss_Seidel(const int n, const float *A, const float *b, float *x)
{
	float *coeMtr = new float [n*n];
	int rowI, colI, addI;
	float temp, mul1, mul2;
	
	//calculate ATA into coeMtr
	for(rowI=0;rowI<n;rowI++)
	{
		temp=0.0f;
		for(addI=0;addI<n;addI++)
		{
			temp+=A[addI*n+rowI]*A[addI*n+rowI];
		}
		coeMtr[rowI*n+rowI]=temp;

		for(colI=0;colI<rowI;colI++)
		{
			temp=0.0f;
			for(int addI=0;addI<n;addI++)
			{
				if(((mul1=A[addI*n+rowI])==0.0f)||((mul2=A[addI*n+colI])==0.0f))
				{
					continue;
				}
				temp+=mul1*mul2;
			}
			coeMtr[rowI*n+colI]=coeMtr[colI*n+rowI]=temp;
		}
	}

	//calculate ATb into x
	for(colI=0;colI<n;colI++)
	{
		temp=0.0f;
		for(rowI=0;rowI<n;rowI++)
		{
			if((mul1=A[rowI*n+colI])==0.0f)
			{
				continue;
			}
			temp+=mul1*b[rowI];
		}
		x[colI]=temp;
	}

	float *mid=new float[n];
	//solve x0 for ATAx = ATb into mid
	Gauss_eliminate(n, coeMtr, x, mid);

	//calculate AAT into coeMtr
	for(rowI=0;rowI<n;rowI++)
	{
		temp=0.0f;
		for(addI=0;addI<n;addI++)
		{
			temp+=A[rowI*n+addI]*A[rowI*n+addI];
		}
		coeMtr[rowI*n+rowI]=temp;

		for(colI=0;colI<rowI;colI++)
		{
			temp=0.0f;
			for(int addI=0;addI<n;addI++)
			{
				if(((mul1=A[rowI*n+addI])==0.0f)||((mul2=A[colI*n+addI])==0.0f))
				{
					continue;
				}
				temp+=mul1*mul2;
			}
			coeMtr[rowI*n+colI]=coeMtr[colI*n+rowI]=temp;
		}
	}
	
	//calculate Ax0 into x
	for(rowI=0;rowI<n;rowI++)
	{
		temp=0.0f;
		for(colI=0;colI<n;colI++)
		{
			if((mul1=A[rowI*n+colI])==0.0f)
			{
				continue;
			}
			temp+=mul1*mid[colI];
		}
		x[rowI]=temp;
	}

	//solve y0 for AATy = Ax0 into mid
	Gauss_eliminate(n, coeMtr, x, mid);

	delete coeMtr;

	//calculate x=ATy0 into x
	for(colI=0;colI<n;colI++)
	{
		temp=0.0f;
		for(rowI=0;rowI<n;rowI++)
		{
			if((mul1=A[rowI*n+colI])==0.0f)
			{
				continue;
			}
			temp+=mul1*mid[rowI];
		}
		x[colI]=temp;
	}
	
	delete mid;
	
	return 0;
}