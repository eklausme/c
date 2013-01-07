/* Generate 10-digit random numbers and count whether generated number
   makes a German tax id or not.

   Elmar Klausmeier, 07-Jan-2013
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>



int myrand (void) {
	static int m=0, r=0;

	r = (m == 0) ? (int)random() : r/10;
	m = (m + 1) % 8;
	return (r % 10);
}


int main (int argc, char *argv[]) {
	FILE *fp;
	int prtflag = 0, taxflag = 0;
	int c, i=0, t=0, l=10, x[10], g[10], r[10], rs[10];
	long n0, n=5;

	g[0] = 0;
	g[1] = 0;
	g[2] = 0;
	g[3] = 0;
	g[4] = 0;
	g[5] = 0;
	g[6] = 0;
	g[7] = 0;
	g[8] = 0;
	g[9] = 0;
	r[0] = 0;
	r[1] = 0;
	r[2] = 0;
	r[3] = 0;
	r[4] = 0;
	r[5] = 0;
	r[6] = 0;
	r[7] = 0;
	r[8] = 0;
	r[9] = 0;
	x[0] = -1;
	x[1] = -1;
	x[2] = -1;
	x[3] = -1;
	x[4] = -1;
	x[5] = -1;
	x[6] = -1;
	x[7] = -1;
	x[8] = -1;
	x[9] = -1;

	while ((c = getopt(argc,argv,"l:n:pt")) != -1) {
		switch (c) {
		case 'l':
			l = atoi(optarg);
			if (l < 3 || l > 10) {
				printf("%s: limit l must be in [3...10]\n",argv[0]);
				return 1;
			}
			break;
		case 'n':
			n = atol(optarg);
			break;
		case 'p':
			prtflag = 1;
			break;
		case 't':
			taxflag = 1;
			break;
		default:
			printf("%s: unknown flag %c\n",argv[0],c);
			return 2;
		}
	}

	if ((fp = fopen("/dev/urandom","r")) == NULL) {
		printf("%s: cannot open /dev/urandom, %s\n",
			argv[0], strerror(errno));
		return 3;
	}
	srandom(fgetc(fp) * 256 + fgetc(fp));

	//n = (10 * n) / 2;	// divide by 2, because we read two digits per byte
	n0 = n;
	n = l * n;
	while (n-- > 0) {
		//if ((c = fgetc(fp)) == EOF) break;
		c = myrand();
		//x[i] = ((c & 0xFF) >> 4) % 10;
		//x[i+1] = (c & 0xFF) % 10;
		//x[i] = ((c & 0xFF) >> 2) % 10;
		x[i] = c % 10;
		g[x[i]] += 1;
		//g[x[i+1]] += 1;
		r[x[i]] += 1;
		//r[x[i+1]] += 1;
		if (i+1 == l) {
			rs[0] = 0;
			rs[1] = 0;
			rs[2] = 0;
			rs[3] = 0;
			rs[4] = 0;
			rs[5] = 0;
			rs[6] = 0;
			rs[7] = 0;
			rs[8] = 0;
			rs[9] = 0;
			rs[r[0]] += 1;
			rs[r[1]] += 1;
			rs[r[2]] += 1;
			rs[r[3]] += 1;
			rs[r[4]] += 1;
			rs[r[5]] += 1;
			rs[r[6]] += 1;
			rs[r[7]] += 1;
			rs[r[8]] += 1;
			rs[r[9]] += 1;
			t += c = (x[0] != 0 && rs[0] == 11-l && rs[1] == l-2 && rs[2] == 1);
			if ((taxflag && c) || prtflag) printf("%c%d %d %d %d %d %d %d %d %d %d / %d%d%d%d%d%d%d%d%d%d\n",
				c?'*':' ',x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7],x[8],x[9],
				r[0],r[1],r[2],r[3],r[4],r[5],r[6],r[7],r[8],r[9]);
			r[0] = 0;
			r[1] = 0;
			r[2] = 0;
			r[3] = 0;
			r[4] = 0;
			r[5] = 0;
			r[6] = 0;
			r[7] = 0;
			r[8] = 0;
			r[9] = 0;
		}
		//i = (i + 2) % 10;
		i = (i + 1) % l;
	}

	puts("Digit\tOccur.\tPerc.");
	for (i=0; i<10; ++i)
		printf("%d\t%d\t%f\n", i, g[i],100.0*g[i]/(l*n0));
	printf("tax ids: %d\t%f\n", t, 100.0*t / n0);

	if (fclose(fp) != 0) {
		printf("%s: cannot close /dev/urandom, %s\n",
			argv[0], strerror(errno));
		return 4;
	}

	return 0;

}

