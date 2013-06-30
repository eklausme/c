/* Generate 10-digit random numbers (BPKENN) and hash them.
   Use EBCDIC alphabet instead of ASCII.
   Count collisions.
   See http://www.strchr.com/hash_functions
   for various hash functions.

   Elmar Klausmeier, 29-Jun-2013
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/sha.h>

#define MAXLEN	50
#define DISTMAX	30


// Taken from dd.c in coreutils-8.13
static unsigned char const ascii_to_ebcdic[] = {
  '\000', '\001', '\002', '\003', '\067', '\055', '\056', '\057',
  '\026', '\005', '\045', '\013', '\014', '\015', '\016', '\017',
  '\020', '\021', '\022', '\023', '\074', '\075', '\062', '\046',
  '\030', '\031', '\077', '\047', '\034', '\035', '\036', '\037',
  '\100', '\117', '\177', '\173', '\133', '\154', '\120', '\175',
  '\115', '\135', '\134', '\116', '\153', '\140', '\113', '\141',
  '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
  '\370', '\371', '\172', '\136', '\114', '\176', '\156', '\157',
  '\174', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
  '\310', '\311', '\321', '\322', '\323', '\324', '\325', '\326',
  '\327', '\330', '\331', '\342', '\343', '\344', '\345', '\346',
  '\347', '\350', '\351', '\112', '\340', '\132', '\137', '\155',
  '\171', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
  '\210', '\211', '\221', '\222', '\223', '\224', '\225', '\226',
  '\227', '\230', '\231', '\242', '\243', '\244', '\245', '\246',
  '\247', '\250', '\251', '\300', '\152', '\320', '\241', '\007',
  '\040', '\041', '\042', '\043', '\044', '\025', '\006', '\027',
  '\050', '\051', '\052', '\053', '\054', '\011', '\012', '\033',
  '\060', '\061', '\032', '\063', '\064', '\065', '\066', '\010',
  '\070', '\071', '\072', '\073', '\004', '\024', '\076', '\341',
  '\101', '\102', '\103', '\104', '\105', '\106', '\107', '\110',
  '\111', '\121', '\122', '\123', '\124', '\125', '\126', '\127',
  '\130', '\131', '\142', '\143', '\144', '\145', '\146', '\147',
  '\150', '\151', '\160', '\161', '\162', '\163', '\164', '\165',
  '\166', '\167', '\170', '\200', '\212', '\213', '\214', '\215',
  '\216', '\217', '\220', '\232', '\233', '\234', '\235', '\236',
  '\237', '\240', '\252', '\253', '\254', '\255', '\256', '\257',
  '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
  '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
  '\312', '\313', '\314', '\315', '\316', '\317', '\332', '\333',
  '\334', '\335', '\336', '\337', '\352', '\353', '\354', '\355',
  '\356', '\357', '\372', '\373', '\374', '\375', '\376', '\377'
};

static unsigned char const ebcdic_to_ascii[] = {
  '\000', '\001', '\002', '\003', '\234', '\011', '\206', '\177',
  '\227', '\215', '\216', '\013', '\014', '\015', '\016', '\017',
  '\020', '\021', '\022', '\023', '\235', '\205', '\010', '\207',
  '\030', '\031', '\222', '\217', '\034', '\035', '\036', '\037',
  '\200', '\201', '\202', '\203', '\204', '\012', '\027', '\033',
  '\210', '\211', '\212', '\213', '\214', '\005', '\006', '\007',
  '\220', '\221', '\026', '\223', '\224', '\225', '\226', '\004',
  '\230', '\231', '\232', '\233', '\024', '\025', '\236', '\032',
  '\040', '\240', '\241', '\242', '\243', '\244', '\245', '\246',
  '\247', '\250', '\133', '\056', '\074', '\050', '\053', '\041',
  '\046', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
  '\260', '\261', '\135', '\044', '\052', '\051', '\073', '\136',
  '\055', '\057', '\262', '\263', '\264', '\265', '\266', '\267',
  '\270', '\271', '\174', '\054', '\045', '\137', '\076', '\077',
  '\272', '\273', '\274', '\275', '\276', '\277', '\300', '\301',
  '\302', '\140', '\072', '\043', '\100', '\047', '\075', '\042',
  '\303', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
  '\150', '\151', '\304', '\305', '\306', '\307', '\310', '\311',
  '\312', '\152', '\153', '\154', '\155', '\156', '\157', '\160',
  '\161', '\162', '\313', '\314', '\315', '\316', '\317', '\320',
  '\321', '\176', '\163', '\164', '\165', '\166', '\167', '\170',
  '\171', '\172', '\322', '\323', '\324', '\325', '\326', '\327',
  '\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
  '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
  '\173', '\101', '\102', '\103', '\104', '\105', '\106', '\107',
  '\110', '\111', '\350', '\351', '\352', '\353', '\354', '\355',
  '\175', '\112', '\113', '\114', '\115', '\116', '\117', '\120',
  '\121', '\122', '\356', '\357', '\360', '\361', '\362', '\363',
  '\134', '\237', '\123', '\124', '\125', '\126', '\127', '\130',
  '\131', '\132', '\364', '\365', '\366', '\367', '\370', '\371',
  '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
  '\070', '\071', '\372', '\373', '\374', '\375', '\376', '\377'
};


int myrand (void) {
	static int m=0, r=0;

	r = (m == 0) ? (int)random() : r/10;	// shift thru digits
	m = (m + 1) % 8;
	return (r % 10);
}

unsigned long hashKR (unsigned char b[], int l) {
	int i;
	unsigned long h = 0;	//5381;
	for (i=0; i<l; ++i) {
		h = 31 * h + b[i];	// Kernighan & Ritchie
		//h = 2 * h + c * c;	// from migration design doc
		//h = 10 * h +  (b[i] - '0');
		//h %= 16777213;		// so that COBOL does not overflow
		//if (h < 0) printf("for-loop hashKR: h=%ld\n", h);
	}
	/* * *
	h ^= h << 3;
	h += h >> 5;
	h ^= h << 4;
	h += h >> 17;
	h ^= h << 25;
	h += h >> 6;
	* * */
	return (h >= 0) ? h : -h;
}

unsigned long SHA1hash (unsigned char b[], int l) {
	unsigned char md[SHA_DIGEST_LENGTH];
	union {
		long h;
		unsigned char c[8];
	} u;

	SHA1(b,l,md);

	u.c[0] = md[12];
	u.c[1] = md[13];
	u.c[2] = md[14];
	u.c[3] = md[15];
	u.c[4] = md[16];
	u.c[5] = md[17];
	u.c[6] = md[18];
	u.c[7] = md[19];

	if (u.h < 0) u.h = -u.h;
	return u.h;
}

unsigned long digitmod (unsigned char b[], int l) {
	int i;
	unsigned long h = 0;
	for (i=0; i<l; ++i)
		h = 10*h + b[i];
	return h;
}

unsigned long hashDesign (unsigned char b[], int l) {
	int i;
	unsigned long h = 0;	//5381;
	for (i=0; i<l; ++i) {
		h = 2 * h + b[i] * b[i];// from migration design doc
		h %= 16777213;		// so that COBOL does not overflow
		if (h < 0) printf("for-loop hashDesign: h=%ld\n", h);
	}
	/* * *
	h ^= h << 3;
	h += h >> 5;
	h ^= h << 4;
	h += h >> 17;
	h ^= h << 25;
	h += h >> 6;
	* * */
	return h;
}


unsigned long (*hf[]) (unsigned char b[], int l) = {
	&hashKR,
	&SHA1hash,
	&digitmod,
	&hashDesign
};


int main (int argc, char *argv[]) {
	FILE *fp;	// filepointer to /dev/urandom
	int prtflag = 0, found;
	int c, i, hfnum=0, l=10, ncol=0, distr[DISTMAX];
	unsigned char b[MAXLEN];
	long h, hashsize=11, k, n=5, n0;
	struct hashtable {
		int coll;	// number of collisions
		unsigned char b[MAXLEN];	// value element of hash
	} *ht;

	while ((c = getopt(argc,argv,"f:h:l:n:pt")) != -1) {
		switch (c) {
		case 'f':	// hash function
			hfnum = atoi(optarg);
			if (hfnum < 0 || hfnum >= sizeof(hf)/sizeof(hf[0])) {
				printf("%s: illegal hash function number %d\n",
					argv[0], hfnum);
				return 1;
			}
			break;
		case 'h':	// max hashtable size
			hashsize = atol(optarg);
			break;
		case 'l':	// length, default is 10
			l = atoi(optarg);
			if (l < 1 || l > 40) {
				printf("%s: limit l must be in [1...40]\n",argv[0]);
				return 2;
			}
			break;
		case 'n':	// number of random numbers to generate
			n = atol(optarg);
			break;
		case 'p':
			prtflag = 1;
			break;
		default:
			printf("%s: unknown flag %c\n",argv[0],c);
			return 3;
		}
	}

	if ((fp = fopen("/dev/urandom","r")) == NULL) {
		printf("%s: cannot open /dev/urandom, %s\n",
			argv[0], strerror(errno));
		return 4;
	}
	srandom(fgetc(fp) * 256 + fgetc(fp));

	if (fclose(fp) != 0) {
		printf("%s: cannot close /dev/urandom, %s\n",
			argv[0], strerror(errno));
		return 5;
	}

	if ((ht = (struct hashtable*)calloc(hashsize,sizeof(struct hashtable))) == NULL) {
		printf("%s: cannot allocate %ld slots for hash table\n",
			argv[0], hashsize);
		return 6;
	}

	// Kernighan & Ritchie: M=31, Init=0
	// Bernstein: M=33, Init=5381
	printf("Number of random numbers: %ld, hash size = %ld\n", n, hashsize);
	n0 = n;
	while (n-- > 0) {
		for (i=0; i<l; ++i)
			b[i] = ascii_to_ebcdic['0' + myrand()];
		h = (*hf[hfnum])(b,l);
		//h = SHA1hash(b,l);
		//h = digitmod(b,l);
		h %= hashsize;
		if (h < 0) printf("error: h=%ld\n",h);

		found = 0;
		for (k=hashsize; k-- > 0; ) {	// scan thru table
			if (ht[h].coll == 0) {
				strncpy(ht[h].b,b,l);	// copy value element to empty slot
				ht[h].coll = 1;	// count collisions for this slot
				found = 1;
				break;
			} else if (strncmp(ht[h].b,b,l) == 0) {
				found = 1;
				break;
			} else {
				ht[h].coll += 1;	// count collisions for this slot
				if (ht[h].coll > 1) ++ncol;	// summation
			}
			h = (h + 1) % hashsize;	// linear probing
		}
		if (found == 0) {
			printf("Did not find: ");
			for (i=0; i<l; ++i) putchar(ebcdic_to_ascii[b[i]]);
			puts("");
		}

		if (prtflag) {
			for (i=0; i<l; ++i) putchar(ebcdic_to_ascii[b[i]]);
			printf(", %8ld, %6d, %4d\n", h, ht[h].coll, ncol);
		}
	}

	// Calculate distribution
	for (i=0; i<DISTMAX; ++i) distr[i] = 0;
	for (n = hashsize; --n >= 0; ) {
		if (ht[n].coll > DISTMAX-1) distr[DISTMAX-1] += 1;
		else distr[ht[n].coll] += 1;
	}

	printf("Total number of collisions: %d = %f%%, "
		"load factor: %f\n",
		ncol, 100.0*ncol/n0, 100.0*n0 / hashsize);
	for (i=0; i<DISTMAX; ++i)
		printf("%2d %8d   %.2f\n", i, distr[i], 100.0*distr[i]/hashsize);

	return 0;

}

