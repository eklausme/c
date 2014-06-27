#include <stdio.h>
//#define FILESZ	1500000000L
#define FILESZ	150L


int main (int argc, char *argv[]) {
	FILE *in, *out=NULL;
	long s = FILESZ, i = 0;
	char fname[64];
	int c;
	fname[0] = '\0';

	if (argc <= 1) return 1;

	printf("Splitting %s\n", argv[1]);
	if ((in = fopen(argv[1],"rb")) == NULL) return 2;

	while ((c = fgetc(in)) != EOF) {
		//printf("%x\n",c);
		if (++s >= FILESZ) {
			s = 0;
			if (out != NULL) fclose(out);
			sprintf(fname,"%s.%03d",argv[1],++i);
			if (i > 50) return 3;
			printf("New split file %s\n",fname);
			if ((out = fopen(fname,"wb")) == NULL) return 4;
		}
		fputc(c,out);
	}


	return 0;
}
