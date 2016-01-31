/* Test whether read(), fread(), or mmap() are faster.
   Elmar Klausmeier, 22-Nov-2015
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define MAXBUF	65536



int freadtst (char *pgname, char *fname) {
	char buf[MAXBUF];
	int i, n, result=0;	// integer definition for i&n crucial for performance
	FILE *fp = fopen(fname, "r");

	if (fp == NULL) {
		printf("%s: cannot open %s\n", pgname,fname);
		exit(2);
	}
	while ( (n = fread(buf,1,MAXBUF,fp)) > 0 )
		for (i=0; i<n; i++)
			result += buf[i];
	if (fclose(fp) != 0) {
		printf("%s: cannot close %s\n", pgname,fname);
		exit(3);
	}
	return result;
}



int mmaptst (char *pgname, char *fname) {
	int fd;
	char *p;
	int i, result=0;
	struct stat sb;

	if ((fd = open(fname,O_RDONLY)) < 0) {
		printf("%s: cannot open %s\n", pgname,fname);
		exit(2);
	}
	if (fstat(fd, &sb) != 0) {
		printf("%s: cannot stat %s\n", pgname,fname);
		exit(4);
	}
	if ((p = mmap(NULL,sb.st_size,PROT_READ,MAP_SHARED|MAP_POPULATE,fd,0)) == MAP_FAILED) {
		printf("%s: cannot memory-map %s\n", pgname,fname);
		exit(5);
	}

	for (i=sb.st_size; i>0; --i)
		result += *p++;

	if (close(fd) < 0) {
		printf("%s: cannot close %s\n", pgname,fname);
		exit(3);
	}
	return result;
}



int readtst (char *pgname, char *fname) {
	int fd;
	char buf[MAXBUF];
	int i, n, result=0;

	if ((fd = open(fname,O_RDONLY)) < 0) {
		printf("%s: cannot open %s\n", pgname,fname);
		exit(2);
	}

	while ( (n = read(fd,buf,MAXBUF)) > 0 )
		for (i=0; i<n; ++i)
			result += buf[i];

	if (close(fd) < 0) {
		printf("%s: cannot close %s\n", pgname,fname);
		exit(3);
	}
	return result;
}



int main (int argc, char* argv[]) {
	int c, fctix=0;
	int (*f[])(char *pgname, char *fname) = { freadtst, mmaptst, readtst };

	while ((c = getopt(argc,argv,"fmr")) != -1) {
		switch (c) {
		case 'f': fctix=0; break;
		case 'm': fctix=1; break;
		case 'r': fctix=2; break;
		default:
			printf("%s: unknown option %c\n", argv[0],c);
			return 1;
		}
	}

	if (optind >= argc) {
		printf("%s: filename expected", argv[0]);
		return 1;
	}
	printf("The answer is: %d\n", f[fctix](argv[0],argv[optind]));
	return 0;
}

