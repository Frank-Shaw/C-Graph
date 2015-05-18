#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <math.h>


using namespace std;

#define MAX_SIZE 5000
//#define bound pow(2, 127)
#define ZERO 0.000000001 /* X is considered to be 0 if |X|<ZERO */


int eliminate_zero(int n, float at[][MAX_SIZE], float b[])
{
	int i, j, k, maxdiarow;
	float maxdia, temp;
	for (i = 0; i<n; i++)
	{
		//downward scan
		maxdia = fabs(at[i][i]);
		maxdiarow = i;
		for (j = i; j<n; j++)
		{
			if (fabs(at[j][i]) > maxdia)
				maxdiarow = j;
		}

		if (at[maxdiarow][i]<ZERO)
		{
			//upward scan
			maxdia = fabs(at[i][i]);
			maxdiarow = i;
			for (j = i; j >= 0; j--)
			{
				if (fabs(at[j][i]) > maxdia)
					maxdiarow = j;
			}
			if (at[maxdiarow][i]<ZERO)//all are the 0
				return -1;
			else
			{
				for (k = 0; k<n; k++)
				{
					at[i][k] = at[i][k] + at[maxdiarow][k];//add the row to this row

				}
				b[i] = b[i] + b[maxdiarow];
			}

		}

		else
		{
			for (k = 0; k<n; k++)
			{
				temp = at[i][k];
				at[i][k] = at[maxdiarow][k];
				at[maxdiarow][k] = temp;

			}
			temp = b[i];
			b[i] = b[maxdiarow];
			b[maxdiarow] = temp;
		}

	}
	return 0;
}






int Gauss_eliminate(int n,float a[][MAX_SIZE], float b[],float x[])
{
	//eliminate the trace that is 0
	int temp = eliminate_zero(n, a, b);
	int i, j, k;

	//eliminate with Gauss
	for (i = 0; i < n - 1; i++)
	{
		if (a[i][i] == 0)
			return -1;
		for (j = i + 1; j < n; j++)
		{
			float div = -1 * a[j][i] / a[i][i];
			for (k = 0; k < n; k++)
				a[j][k] += (a[i][k] * div);
			b[j] += (b[i] * div);
		}
	}


	//solve the x
	for (i = n - 1; i >= 0; i--)
	{
		if (a[i][i] == 0)
			x[i] = 0;
		else
		{
			float sum = 0;
			for (j = n - 1; j > i; j--)
				sum += a[i][j] * x[j];
			x[i] = (b[i] - sum) / a[i][i];
		}
		

	}


	return 0;
}


/*
int judge(int n, float at[][MAX_SIZE], float b[])
{
	int i,j,k,maxdiarow;
	float maxdia,temp;
	for(i=0;i<n;i++)
	{
			//downward scan
			maxdia=fabs(at[i][i]);
			maxdiarow=i;
			for(j=i;j<n;j++)
			{
				if(fabs(at[j][i]) > maxdia)
					maxdiarow=j;
			}
			
			if(at[maxdiarow][i]<ZERO)
			{
				//upward scan
				maxdia=fabs(at[i][i]);
				maxdiarow=i;
				for(j=i;j>=0;j--)
				{
					if(fabs(at[j][i]) > maxdia)
						maxdiarow=j;
				}
				if(at[maxdiarow][i]<ZERO)//all are the 0
					return -1;
				else
				{
					for(k=0;k<n;k++)
					{
						at[i][k]=at[i][k]+at[maxdiarow][k];//add the row to this row
						
					}
					b[i]=b[i]+b[maxdiarow];
				}
				
			}
			
			else
			{
				for(k=0;k<n;k++)
				{
					temp=at[i][k];
					at[i][k]=at[maxdiarow][k];
					at[maxdiarow][k]=temp;
					
				}
				temp=b[i];
				b[i]=b[maxdiarow];
				b[maxdiarow]=temp;
			}
			
	}
	return 0;
}

 */
 


int Gauss_Seidel(int n, float a[][MAX_SIZE], float b[], float x[])
{
	int i,j,k;
	float aT[MAX_SIZE][MAX_SIZE], aTa[MAX_SIZE][MAX_SIZE], aTb[MAX_SIZE];
	
	//copy a to aT
	for (i = 0; i < n;i++)
	for (j = 0; j < n; j++)
		aT[i][j] = a[i][j];

	for (i = 0; i < n; i++)
	{
		for (j = i + 1; j < n; j++)
		{
			float temp;
			temp = aT[i][j];
			aT[i][j] = aT[j][i];
			aT[j][i] = temp;
		}
	}


	//calculate the ATA
	for (i = 0; i < n; i++)
	{
		for (j = 0; j < n; j++)
		{
			float temp = 0;
			for (k = 0; k < n; k++)
				temp += aT[i][k] * a[k][j];
			aTa[i][j] = temp;
		}
	}


	//calculate the ATb
	for (i = 0; i < n; i++)
	{
		float temp = 0;
		for (j = 0; j < n; j++)
			temp += aT[i][j] * b[j];
		aTb[i] = temp;
	}

	float x0[MAX_SIZE];

	int temp = Gauss_eliminate(n,aTa,aTb,x0);
	//it can solve for x0

	//aTa is end and use for aaT
	//aTb is end and use for ax0
	
	
	//float aaT[MAX_SIZE][MAX_SIZE], ax0[MAX_SIZE];

	//calculate the aaT
	for (i = 0; i < n; i++)
	{
		for (j = 0; j < n; j++)
		{
			float temp = 0;
			for (k = 0; k < n; k++)
				temp += a[i][k] * aT[k][j];
			//aaT[i][j] = temp;
			aTa[i][j] = temp;
		}
	}


	//calculate the ax0
	for (i = 0; i < n; i++)
	{
		float temp = 0;
		for (j = 0; j < n; j++)
			temp += a[i][j] * x0[j];
		//ax0[i] = temp;
		aTb[i] = temp;
	}

	//x0 is end and use for y0
	//float y0[MAX_SIZE];

	//temp = Gauss_eliminate(n, aaT, ax0, y0);
	temp = Gauss_eliminate(n, aTa, aTb, x0);


	//it can solve for y0

	

	//calculate the x
	for (i = 0; i < n; i++)
	{
		float temp = 0;
		for (j = 0; j < n; j++)
			//temp += aT[i][j] * y0[j];
			temp += aT[i][j] * x0[j];
		x[i] = temp;
	}
	

	return 0;
}



int main()
{
	int n, i, j, k;
	//float a[MAX_SIZE][MAX_SIZE], b[MAX_SIZE], x[MAX_SIZE];
	float a[MAX_SIZE][MAX_SIZE] = { { 2, -1, 1 }, { 2, 2, 2 }, { -1, -1, 2, } }, b[MAX_SIZE] = { -1, 4, -5, }, x[MAX_SIZE];
	/*
	while (scanf("%d", &n) != EOF) {
		for (i = 0; i<n; i++) {
			for (j = 0; j<n; j++)
				scanf("%f", &a[i][j]);
			scanf("%f", &b[i]);
		}
		*/
		
	//test
	{
		n = 3;


		


		printf("Result of Gauss-Seidel method:\n");
		for (i = 0; i<n; i++)
			x[i] = 0.0;
		k = Gauss_Seidel(n, a, b, x);
		printf("k=%d\n", k);
		printf("\n");
	}

	for (i = 0; i < n; i++)
		printf("%.4lf\n", x[i]);

	system("pause\n");
	return 0;
}

