/* Evaluate difference scheme.
   It has generating polynomial
             4      2
            x  - 2 x  + 1

   and has real roots:
                  2        2
           (x - 1)  (x + 1)

   So all values should remain bounded.

   Floating point case: Polynomial is
             3        2
     4   25 x    311 x    29 x   14
    x  - ----- - ------ + ---- + --
          72      216     108    27

   with roots
                 8       2       7
    (x - 1) (x - -) (x + -) (x + -)
                 9       3       8


   Elmar Klausmeier, 29-Dec-2012
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>


int main (int argc, char *argv[]) {
	register int i, x[5];
	int c, i0=0, m=1, xin[5], fp=0, prtflag = 0;
	double xd[5];

	while ((c = getopt(argc,argv,"fm:n:p")) != -1) {
		switch (c) {
		case 'f':
			fp = 1;
			break;
		case 'm':
			m = atoi(optarg);
			break;
		case 'n':
			i0 = atoi(optarg);
			break;
		case 'p':
			prtflag = 1;
			break;
		default:
			printf("%s: unknown flag %c\n",argv[0],c);
			return 1;
		}
	}

	if (i0 == 0) i0 = INT_MAX;
	if (m == 0) m = 1;
	if (fp) goto fp_section;

	// Integer computation
	if ((c = scanf("%d %d %d %d", &xin[0],&xin[1],&xin[2],&xin[3])) != 4) {
		printf("%s: Need exactly 4 input arguments via stdin, read only %d\n",argv[0],c);
		return 3;
	}
	x[0] = xin[0];
	x[1] = xin[1];
	x[2] = xin[2];
	x[3] = xin[3];
	//x[4] = xin[4];

	while (m-- > 0) {
		i = i0;
		while (i-- > 0) {
			x[4] = 2*x[2] - x[0];
#ifdef PRTFLAG
			//x[4] = (75*x[3] + 311*x[2] - 58*x[1] - 112*x[0]) / 216;
			if (prtflag || x[4] > 9999999999 || x[4] < -9999999999)
				printf("%d\n",x[4]);
#endif
			x[0] = x[1];
			x[1] = x[2];
			x[2] = x[3];
			x[3] = x[4];
			//x[4] = x[5];
		}
	}
	printf("%d\n",x[4]);

	return 0;

fp_section:
	// Floating-point computation
	if ((c = scanf("%lf %lf %lf %lf %lf", &xd[0],&xd[1],&xd[2],&xd[3],&xd[4])) != 4) {
		printf("%s: Need exactly 4 input arguments via stdin, read only %d\n",argv[0],c);
		return 3;
	}
	while (m-- > 0) {
		i = i0;
		while (i-- > 0) {
			xd[4] = 25.0/72*xd[3] + 311.0/216*xd[2] - 29.0/108*xd[1] - 14.0/27*xd[0];
#ifdef PRTFLAG
			if (prtflag || xd[4] > 9999999999 || xd[4] < -9999999999)
				printf("%f\n",xd[4]);
#endif
			xd[0] = xd[1];
			xd[1] = xd[2];
			xd[2] = xd[3];
			xd[3] = xd[4];
		}
	}
	printf("%f\n",xd[4]);


	return 0;

}

