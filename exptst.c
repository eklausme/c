/* Test speed of single precision exp(),
   double precision exp()
   quadruple precision exp()

   Elmar Klausmeier, 05-May-2020
*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	long i, rep=1024, n=65000;
	int c, precision='d';
	float sf = 0;
	double sd = 0;
	long double sq = 0;

	while ((c = getopt(argc,argv,"dfqn:r:")) != -1) {
		switch (c) {
		case 'd':
			precision = 'd';
			break;
		case 'f':
			precision = 'f';
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'q':
			precision = 'q';
			break;
		case 'r':
			rep = atoi(optarg);
			if (rep <= 0) return 2;
			break;
		default:
			printf("%s: unknown flag %c\n", argv[0], c);
			return 1;
		}
	}

	switch(precision) {
	case 'd':
		while (rep-- > 0)
			for (i=0; i<n; ++i)
				sd += exp(i % 53) - exp((i+1) % 43) - exp((i+2) % 47) - exp((i+3) % 37);
		printf("sd = %f\n",sd);
		break;
	case 'f':
		while (rep-- > 0)
			for (i=0; i<n; ++i)
				sf += expf(i % 53) - expf((i+1) % 43) - expf((i+2) % 47) - expf((i+3) % 37);
		printf("sf = %f\n",sf);
		break;
	case 'q':
		while (rep-- > 0)
			for (i=0; i<n; ++i)
				sq += expl(i % 53) - expl((i+1) % 43) - expl((i+2) % 47) - expl((i+3) % 37);
		printf("sq = %Lf\n",sq);
		break;
	}

	return 0;
}

