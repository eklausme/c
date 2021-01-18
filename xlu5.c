#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>


//#define DEBUGMEDIUM	1
//#define P_PROFILE	1


typedef int index_t;
typedef double real;



#ifdef P_PROFILE
enum FunctionNr {
	P_FirstFunction,
	P_decomp, P_solve, P_maxnorm, P_l2norm, P_cpvec, P_xxmy,
	P_YAX, P_main,
	P_LastFunction
};



static char *FunctionName[] = {
	"First Function",
	"decomp", "solve", "maxnorm", "l2norm", "cpvec", "xxmy",
	"YAX", "main",
	"Last Function"
};

class Profile {
	static int last;                          // last actual function
	static int stack[P_LastFunction];         // stack of indices
	static int stackindex;                    // stack index
	static int ncalls[P_LastFunction];        // number of calls
	static clock_t lastTime[P_LastFunction];  // last time the routine was active
	static clock_t accTime[P_LastFunction];   // accumulated time
	void Push (int i) {  stack[stackindex++] = i;  }
	int Pop (void) {  return (stack[--stackindex]);  }
public:
	Profile (FunctionNr i);
	~Profile (void);
	void Print (void);
};

int Profile::last;                          // last actual function
int Profile::stack[P_LastFunction];         // stack of indices
int Profile::stackindex;                    // stack index
int Profile::ncalls[P_LastFunction];        // number of calls
clock_t Profile::lastTime[P_LastFunction];  // last time the routine was active
clock_t Profile::accTime[P_LastFunction];   // accumulated time



inline Profile::Profile (FunctionNr i) {
	ncalls[i] += 1;
	clock_t t = clock();
	accTime[last] += t - lastTime[last];
	lastTime[i] = t;
	Push(last);
	last = i;
}


inline Profile::~Profile (void) {
	clock_t t = clock();
	accTime[last] += t - lastTime[last];
	lastTime[last=Pop()] = t;
}


void Profile::Print (void) {
        accTime[last] += clock() - lastTime[last];

	puts("\n   #     ncalls   Promille        ticks   Promille      function-name");
	int sumCalls = 0;
	clock_t sumTime = 0;

	int i;
	for (i=1; i<P_LastFunction; ++i) {
		sumCalls += ncalls[i];
		sumTime += accTime[i];
	}
	if (sumCalls == 0) sumCalls = -1;
	if (sumTime == 0) sumTime = -1;

	/* Multiplying accTime[i] by 1000 may overflow. So, if possible, we divide the divisor. */
	int overflowFlag = 0;
	long sumTime2 = sumTime;
	if (sumTime % 1000L == 0) {
		sumTime2 = sumTime / 1000L;
		overflowFlag = 1;
	}

	long callProSum=0, timeProSum=0;
	for (i=1; i<P_LastFunction; ++i) {
		long callPromille = (1000L * ncalls[i]) / sumCalls;
		callProSum += callPromille;
		long timePromille = overflowFlag ?  accTime[i] / sumTime2 :  (1000L * accTime[i]) / sumTime;
		timeProSum += timePromille;
		printf(" %3d %10ld %10ld   %10ld %10ld      %s\n", i, (long)ncalls[i], callPromille, (long)accTime[i], timePromille, FunctionName[i]);
	}

	printf("sum: %10ld %10ld   %10ld %10ld\n", (long)sumCalls, callProSum, (long)sumTime, timeProSum);
	long clocksSec = (long) CLOCKS_PER_SEC;
	if (clocksSec) printf("secs = %ld\n", (long)(sumTime) / clocksSec);
}


static Profile P_Dummy(P_FirstFunction);

#else
#ifdef DDT
class Profile {
public:
	Profile (FunctionNr) {}
	~Profile (void) {}
	void Print (void) {}
};
#endif
#define Profile
#define P_(x)
#endif



inline real dabs (real x) { return fabs(x); }



void ErrorExit (const char *mess) {
	puts (mess);
	exit (-1);
}





/* decomp : ...
   Input  : n, n0
            a[0..n0-1][0..n-1]
   Output : ip[0..n-1], ip[n-1]=detsign
   return : FULLRANK (=0) if all is well

   local  : i, j, k, kp1, nm1,  t
   calls  : dabs()

   If the matrix A has maximal rank it will be factored in its lower
   matrix L and upper matrix U which are stored in A.
   If the matrix hasn't full rank the decomposition will prematurely
   abort the calculation and so the factors L and U are undefined.
   In this case, i.e., the matrix is not invertible, solve() will
   divide by zero.
*/

index_t decomp (index_t n, index_t n0, real a[], index_t ip[]) {
	real  t;
	index_t nm1, k, kp1, m;
	register real *ank;
	register real *anj;
	register index_t i;
	register index_t j;

	Profile P_(P_decomp);
#ifdef DEBUGMEDIUM
	if (n <= 0)  ErrorExit("decomp: n <= 0");
	if (n0 <= 0) ErrorExit("decomp: n0 <= 0");
	if (n0 < n)  ErrorExit("decomp: n0 < n");
#endif
	nm1 = n - 1;
	ip[nm1] = 1;

	if (n > 1) {
		for (k=0; k<nm1; ++k) {
			kp1 = k + 1;  /* defined for efficiency only */
			ank = a  +  k * n0;  /* k-th column */

			/* In the k-th column beginning at the (k+1)-th row the
			   biggest element in that column (=pivot) is searched.
			*/
			m = k;
			for (i=kp1; i<n; ++i)  if ( dabs(ank[i]) > dabs(ank[m]) )  m = i;

			ip[k] = m;  t = ank[m];

			/* exchange step:  */
			if (m != k) {
				ip[nm1] *= -1;
				ank[m] = ank[k];
				ank[k] = t;
			}

			/* Was there a column with zero elements only? */
			if (t == 0.0e0) {  ip[nm1] = 0;  return kp1;  }    /* rank is k+1 */

			t = (-1.0e0) / t;
			for (i=kp1; i<n; ++i)  ank[i] *= t;

			for (j=kp1; j<n; ++j) {
				anj = a  +  j * n0;
				t = anj[m];      /* swap a_{mj} and a_{kj} */
				anj[m] = anj[k];
				anj[k] = t;
				if (t != 0.0e0)
					for (i=kp1; i<n; ++i)  anj[i] += ank[i] * t;  /* a_{ij} += a_{ik} t */
			}
		}
	}

	if (a[nm1 + nm1*n0] == 0.0e0) {  ip[nm1] = 0;  return n;  }

	return 0;
}





/* solve : ...
   Input  : n, n0, ip[0..n-1],
            a[0..n0-1][0..n-1]
   In/Out : b[0..n-1]

   local  : nm1, k, m, i, kb, km1, nk,  t
   The vector b is overwritten with the solution of Ax=b.
   See the description for decomp() for future developments.
   There is room for improvement concerning efficiency.
   The whole algorithm is column-oriented.
*/

void solve (index_t n, index_t n0, real a[], real b[], index_t ip[]) {
	index_t nm1, m;
	register real *ank;
	register index_t i;
	register index_t k;
	real  t;

	Profile P_(P_solve);
#ifdef DEBUGMEDIUM
	/*chkpivot(n,n0,ip,"solve");*/
	if (ip[n - 1] == 0) ErrorExit
		("solve: Determinant of matrix is zero, so cannot divide.");
#endif
	if (n > 1) {
		nm1 = n - 1;
		/* forward substitution:  */
		for (k=0; k<nm1; ++k) {
			m = ip[k];
			t = b[m];  b[m] = b[k];  b[k] = t;  /* swap b[m] and b[k] */
			ank = a  +  k * n0;
			for (i=k+1; i<n; ++i)  b[i] += ank[i] * t;  /* b_i += a_{ik} t */
		}

		/* backward substitution:  */
		for (k=nm1; k>0; --k) {
			ank  = a  +  k * n0;  /* CPRED(...) */
			b[k] /= ank[k];
			t    = -b[k];
			for (i=0; i<k; ++i)  b[i] += ank[i] * t;  /* b_i += a_{ik} t */
		}
	}

	b[0] /= a[0+0];
}





real maxnorm (index_t n, real *v) {
	Profile P_(P_maxnorm);
	real t, m = dabs(*v);

	while (--n > 0)
		if (m < (t=dabs(*++v)))  m = t;

	return m;
}



real l2norm (index_t n, real *v) {
	Profile P_(P_l2norm);
	real s = 0, t;
	real m = maxnorm(n,v);
	if (m <= 0) return 0;

	while (n-- > 0) {
		t = *v++ / m;
		s += t * t;
	}

	return (m * sqrt(s));
}



#ifdef DDT
void cpvec (index_t n, real *a, real *b) {  /* a = b, for vectors a, b */
	Profile P_(P_cpvec);
	while (n-- > 0)  *a++ = *b++;
}



void xxmy (index_t n, real *x, real *y) {  /* x -= y, for vectors x, y */
	Profile P_(P_xxmy);
	while (n-- > 0)  *x++ -= *y++;
}
#endif



void YAX (index_t n, real *Y, real *A, real *X) {  /* Y = A X,  for matrices Y, A, X */
	Profile P_(P_YAX);
	int i, j, k, nj;
	real t;

	for (i=0; i<n; ++i) {
		nj = 0;
		for (j=0; j<n; ++j) {
			t = 0;
			for (k=0; k<n; ++k)  t += A[i+k*n] * X[k+nj];
			Y[i+nj] = t;
			nj += n;  /* nj == n * j */
		}
	}
}



real *realvecAlloc (index_t n) {
	real *p;
	if ((p = (real *) malloc(sizeof(real) * n)) == NULL) ErrorExit
		("realvecAlloc: malloc() failure.");
	return p;
}



index_t *indvecAlloc (index_t n) {
	index_t *p;
	if ((p = (index_t *) malloc(sizeof(index_t) * n)) == NULL) ErrorExit
		("indvecAlloc: malloc() failure.");
	return p;
}


/*
void matprint (index_t n, real A[]) {
	index_t i, j;

	puts("");
	for (i=0; i<n; ++i) {
		for (j=0; j<n; ++j)
			printf("\t%e", A[i+n*j]);
		puts("");
	}
}
*/



real diffHilb (index_t n, real H[], real A[], real X[], real Y[], index_t ip[]) {
	index_t i, j, nsq;

	nsq = n * n;

	/* H = Hilbert matrix of order n */
	for (i=0; i<n; ++i) {
		for (j=0; j<n; ++j)
			H[i+j*n] = 1.0 / (i + j + 1);
	}
	memcpy(A,H,sizeof(real) * nsq);  /* A = H */

	/* X = identity matrix of order n */
	memset(X, 0, sizeof(real) * nsq);
	for (i=0; i<n; ++i)
		X[i+n*i] = 1;

	if (decomp(n,n,A,ip) > 0)  ErrorExit("Matrix singular.");
	for(i=0; i<n; ++i)
		solve(n,n,A,&X[n*i],ip);
	YAX(n,Y,H,X);
	for (i=0; i<n; ++i)
		Y[i+n*i] -= 1;

	return maxnorm(nsq,Y);
}





int main (int argc, char *argv[]) {
	//Profile Prof(P_main);
	real *A, *X, *Y, *H;
	index_t n=0, nsq, *ip;
	int i;

	if (argc > 1) n = atoi(argv[1]);
	if (n <= 0) n = 451;
	nsq = n * n;
	ip = indvecAlloc(n);
	A = realvecAlloc(nsq); puts("A o.k.");
	H = realvecAlloc(nsq); puts("H o.k.");
	X = realvecAlloc(nsq); puts("X o.k.");
	Y = realvecAlloc(nsq); puts("Y o.k.");

	for (i=1; i<=n; ++i)
		printf("%e%s", diffHilb(i,H,A,X,Y,ip), i%5==0? "\n" : "   ");

	//Prof.Print();
	return 0;
}

