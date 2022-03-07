/* Simple random numbers following Poisson distribution using GSL using parameter lambda.
   Combine with lognormal distribution.
   Parameters for lognormal distribution are provided via:
      1. modal value
      2. either 95% or 99% percentile quantile

   Compile with:
       cc -Wall -O3 -march=native gslSoris.c -o gslSoris -lgsl -lcblas -lm

   Elmar Klausmeier, 02-Feb-2021
   Elmar Klausmeier, 04-Feb-2021: Andreas Schneider comments
   Elmar Klausmeier, 09-Feb-2021: min/max, inverfl()
   Elmar Klausmeier, 11-Feb-2021: Andreas Schneider on Poisson distribution
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>	// for atoi(), atof(), qsort()
#include <math.h>	// for sqrt(), log()
#include <gsl/gsl_randist.h>

#define MAX_DRAW	5000000
//#define MAX_DRAW_ARR	6000000
#define	R1	1.64485362695147
#define R2	2.32634787404084


/* * *
// https://dlmf.nist.gov/7.17, not very precise
double inverf(double x) {
	if (fabs(x) >= 1) exit(98);
	double t = 0.88622692545275801364 * x;	// 0.5*sqrt(pi)*x
	double tsq = t*t;
	return t * (1 + tsq*(1.0/3 + tsq*(7/30.0 + tsq*127.0/630)));
}
* * */


/* * *
// Single precision only, not precise enough
// https://stackoverflow.com/questions/27229371/inverse-error-function-in-c
// compute inverse error functions with maximum error of 2.35793 ulp
float finverf(float a) {
	float p, t;
	t = fmaf (a, 0.0f - a, 1.0f);
	//t = my_logf (t);
	t = logf(t);
	if (fabsf(t) > 6.125f) { // maximum ulp error = 2.35793
		p =              3.03697567e-10f; //  0x1.4deb44p-32 
		p = fmaf (p, t,  2.93243101e-8f); //  0x1.f7c9aep-26 
		p = fmaf (p, t,  1.22150334e-6f); //  0x1.47e512p-20 
		p = fmaf (p, t,  2.84108955e-5f); //  0x1.dca7dep-16 
		p = fmaf (p, t,  3.93552968e-4f); //  0x1.9cab92p-12 
		p = fmaf (p, t,  3.02698812e-3f); //  0x1.8cc0dep-9 
		p = fmaf (p, t,  4.83185798e-3f); //  0x1.3ca920p-8 
		p = fmaf (p, t, -2.64646143e-1f); // -0x1.0eff66p-2 
		p = fmaf (p, t,  8.40016484e-1f); //  0x1.ae16a4p-1 
	} else { // maximum ulp error = 2.35456
		p =              5.43877832e-9f;  //  0x1.75c000p-28 
		p = fmaf (p, t,  1.43286059e-7f); //  0x1.33b458p-23 
		p = fmaf (p, t,  1.22775396e-6f); //  0x1.49929cp-20 
		p = fmaf (p, t,  1.12962631e-7f); //  0x1.e52bbap-24 
		p = fmaf (p, t, -5.61531961e-5f); // -0x1.d70c12p-15 
		p = fmaf (p, t, -1.47697705e-4f); // -0x1.35be9ap-13 
		p = fmaf (p, t,  2.31468701e-3f); //  0x1.2f6402p-9 
		p = fmaf (p, t,  1.15392562e-2f); //  0x1.7a1e4cp-7 
		p = fmaf (p, t, -2.32015476e-1f); // -0x1.db2aeep-3 
		p = fmaf (p, t,  8.86226892e-1f); //  0x1.c5bf88p-1 
	}
	return a * p;
}
* * */



// From hkshayg", https://github.com/lakshayg/erfinv/blob/master/erfinv.c
// Inverse erf() in long double
long double inverfl(long double x) {
	if (x < -1 || x > 1) {
		return NAN;
	} else if (x == 1.0) {
		return INFINITY;
	} else if (x == -1.0) {
		return -INFINITY;
	}

	const long double LN2 = 6.931471805599453094172321214581e-1L;

	const long double A0 = 1.1975323115670912564578e0L;
	const long double A1 = 4.7072688112383978012285e1L;
	const long double A2 = 6.9706266534389598238465e2L;
	const long double A3 = 4.8548868893843886794648e3L;
	const long double A4 = 1.6235862515167575384252e4L;
	const long double A5 = 2.3782041382114385731252e4L;
	const long double A6 = 1.1819493347062294404278e4L;
	const long double A7 = 8.8709406962545514830200e2L;

	const long double B0 = 1.0000000000000000000e0L;
	const long double B1 = 4.2313330701600911252e1L;
	const long double B2 = 6.8718700749205790830e2L;
	const long double B3 = 5.3941960214247511077e3L;
	const long double B4 = 2.1213794301586595867e4L;
	const long double B5 = 3.9307895800092710610e4L;
	const long double B6 = 2.8729085735721942674e4L;
	const long double B7 = 5.2264952788528545610e3L;

	const long double C0 = 1.42343711074968357734e0L;
	const long double C1 = 4.63033784615654529590e0L;
	const long double C2 = 5.76949722146069140550e0L;
	const long double C3 = 3.64784832476320460504e0L;
	const long double C4 = 1.27045825245236838258e0L;
	const long double C5 = 2.41780725177450611770e-1L;
	const long double C6 = 2.27238449892691845833e-2L;
	const long double C7 = 7.74545014278341407640e-4L;

	const long double D0 = 1.4142135623730950488016887e0L;
	const long double D1 = 2.9036514445419946173133295e0L;
	const long double D2 = 2.3707661626024532365971225e0L;
	const long double D3 = 9.7547832001787427186894837e-1L;
	const long double D4 = 2.0945065210512749128288442e-1L;
	const long double D5 = 2.1494160384252876777097297e-2L;
	const long double D6 = 7.7441459065157709165577218e-4L;
	const long double D7 = 1.4859850019840355905497876e-9L;

	const long double E0 = 6.65790464350110377720e0L;
	const long double E1 = 5.46378491116411436990e0L;
	const long double E2 = 1.78482653991729133580e0L;
	const long double E3 = 2.96560571828504891230e-1L;
	const long double E4 = 2.65321895265761230930e-2L;
	const long double E5 = 1.24266094738807843860e-3L;
	const long double E6 = 2.71155556874348757815e-5L;
	const long double E7 = 2.01033439929228813265e-7L;

	const long double F0 = 1.414213562373095048801689e0L;
	const long double F1 = 8.482908416595164588112026e-1L;
	const long double F2 = 1.936480946950659106176712e-1L;
	const long double F3 = 2.103693768272068968719679e-2L;
	const long double F4 = 1.112800997078859844711555e-3L;
	const long double F5 = 2.611088405080593625138020e-5L;
	const long double F6 = 2.010321207683943062279931e-7L;
	const long double F7 = 2.891024605872965461538222e-15L;

	long double abs_x = fabsl(x);

	if (abs_x <= 0.85L) {
		long double r = 0.180625L - 0.25L * x * x;
		long double num = (((((((A7 * r + A6) * r + A5) * r + A4) * r + A3) * r + A2) * r + A1) * r + A0);
		long double den = (((((((B7 * r + B6) * r + B5) * r + B4) * r + B3) * r + B2) * r + B1) * r + B0);
		return x * num / den;
	}

	long double r = sqrtl(LN2 - logl(1.0L - abs_x));

	long double num, den;
	if (r <= 5.0L) {
		r = r - 1.6L;
		num = (((((((C7 * r + C6) * r + C5) * r + C4) * r + C3) * r + C2) * r + C1) * r + C0);
		den = (((((((D7 * r + D6) * r + D5) * r + D4) * r + D3) * r + D2) * r + D1) * r + D0);
	} else {
		r = r - 5.0L;
		num = (((((((E7 * r + E6) * r + E5) * r + E4) * r + E3) * r + E2) * r + E1) * r + E0);
		den = (((((((F7 * r + F6) * r + F5) * r + F4) * r + F3) * r + F2) * r + F1) * r + F0);
	}

	return copysignl(num / den, x);
}



int dblcmp(const void *d1, const void*d2) {	// needed for qsort()
	double t = *((double*)d1) - *((double*)d2);
	if (t < 0) return -1;
	else if (t > 0) return 1;
	return 0;
}


static double drw[MAX_DRAW];


int main (int argc, char *argv[]) {
	int c, i, j, n=20, debug=0, quantileType=95, pois;
	double lambda=2, modal=0, quantile=0, mu=0, sigma=1, minlog=-1, maxlog=-1, x;
	const gsl_rng_type *T;
	gsl_rng *r;

	//if (argc >= 2) n = atoi(argv[1]);
	//if (argc >= 3) lambda = atof(argv[2]);

	while ((c = getopt(argc,argv,"a:b:dhl:m:n:p:q:")) != -1) {
		switch (c) {
			case 'a':
				minlog = atof(optarg);
				break;
			case 'b':
				maxlog = atof(optarg);
				break;
			case 'd':
				debug = 1;
				break;
			case 'h':
				printf(
"%s: Poisson lognormal random number generator\n"
"-a<min>         minimum for lognormal distribution\n"
"-b<max>         maximum for lognormal distribution\n"
"-d              switch on debug output\n"
"-h              this help\n"
"-l<lambda>      set lambda in Poisson distribution, default is 2\n"
"-m<modal>       set modal value in lognormal distribution\n"
"-n<number>      set number of draws taken at least, can be more, default is 20\n"
"-p<percentile>  set 95%% percentile in lognormal distribution\n"
"-q<percentile>  set 99%% percentile in lognormal distribution\n"
"Written by Elmar Klausmeier, February 2021\n",
				argv[0]);
				return 0;
			case 'l':	// Poisson lambda
				lambda = atof(optarg);
				break;
			case 'm':	// modal value
				modal = atof(optarg);
				break;
			case 'n':	// number of draws
				n = atoi(optarg);
				break;
			case 'p':	// 95% percentile
				quantileType = 95;
				quantile = atof(optarg);
				break;
			case 'q':	// 99% percentile
				quantileType = 99;
				quantile = atof(optarg);
				break;
			default:
				printf("Unknown option %c, try -h for help\n", c);
				return 1;
			}
	}
	if (n > MAX_DRAW) {
		printf("Number of draws too large, read %d\n",n);
		return 2;
	}
	if (modal <= 0) {
		printf("Modal non-positive, read %f\n", modal);
		return 3;
	}
	if (quantile <= 0) {
		printf("Quantile (%d%%) zero or negative, read %f\n",quantileType,quantile);
		return 4;
	}
	if (minlog == 0) {
		printf("minlog must be positive, if set, minlog=%f\n",minlog);
		return 5;
	}
	if (maxlog == 0.0) {
		printf("maxlog must be positive, if set, maxlog=%f\n",maxlog);
		return 6;
	}
	//if (optind < argc) ncnt = atoi(argv[optind]);

	// Formula according 20210127_Anfrage von Huang-AW_RCb1.xlsx
	// mu = ln(f4)+(-((f3=0.95)*R1+(f3=0.99)*R2)/2+((((f3=0.95)*R1+(f3=0.99)*R2)/2)^2-ln(f4/f2))^0.5)^2
	// sigma = -((f3=0.95)*R1+(f3=0.99)*R2)/2+((((f3=0.95)*R1+(f3=0.99)*R2)/2)^2-ln(f4/f2))^0.5
	// See https://en.wikipedia.org/wiki/Log-normal_distribution
	if (quantileType == 95) {
		sigma = -R1/2 + sqrt(R1*R1/4 - log(modal/quantile));
	} else if (quantileType == 99) {
		sigma = -R2/2 + sqrt(R2*R2/4 - log(modal/quantile));
	}
	mu = log(modal) + sigma * sigma;
	if (mu <= 0) {
		printf("mu non-positive, mu=%f\n",mu);
		return 7;
	}
	printf("lambda=%.8f, mu=%.8f, sigma=%.8f\n", lambda, mu, sigma);
	printf("Check: median=%.8f, modal=%.8f, q95=%.8f, q99=%.8f\n",
		exp(mu), exp(mu-sigma*sigma),
		exp(mu + M_SQRT2*sigma*inverfl(2*0.95-1)),
		exp(mu + M_SQRT2*sigma*inverfl(2*0.99-1))
	);

	gsl_rng_env_setup();
	T = gsl_rng_default;
	r = gsl_rng_alloc(T);

	for (i=0; i<n; ++i) {
		pois = gsl_ran_poisson(r,lambda);
		drw[i] = 0;	// event did not happen, therefore zero drw[i]
		for (j=0; j<pois; ++j) {	// draw pois-many lognormal random numbers
			for (;;) {
				x = gsl_ran_lognormal(r,mu,sigma);
				if (minlog >= 0.0  &&  x < minlog) continue;
				if (maxlog >= 0.0  &&  x > maxlog) continue;
				break;
			}
			drw[i] += x;
		}
		if (debug) printf("%d\t%d\t%f\n",i+1,pois,drw[i]);
	}

	gsl_rng_free(r);

	qsort(drw,n,sizeof(drw[0]),dblcmp);
	if (debug) puts("Sorted:");
	double sum=0;
	for (i=0; i<n; ++i) {
		sum += drw[i];
		if (debug) printf("%d\t%f\n",i+1,drw[i]);
	}
	int nq05 = (int)(0.05 * n);
	int nq25 = (int)(0.25 * n);
	int nq50 = (int)(0.50 * n);
	int nq95 = (int)(0.95 * n);
	int nq99 = (int)(0.99 * n);
	int nq99d9 = (int)(0.999 * n);
	printf("n=%u, mean=%.8f, q05=%.8f, q25=%.8f, q50=%.8f, q95=%.8f, q99=%.8f, q99d9=%.8f, min=%.8f, max=%.8f\n",
		n, sum/n,
		drw[nq05], drw[nq25], drw[nq50], drw[nq95], drw[nq99], drw[nq99d9],
		drw[0], drw[n-1]);

	return 0;
}

