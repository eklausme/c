/* Convert arbitrary to hex.
   Call with either "-a" to convert from hex to arbitrary,
   or "-h" to convert an arbitrary file to hex.

   Elmar Klausmeier, 12-May-2016
*/

#include <stdio.h>
#include <ctype.h>

int main (int argc, char *argv[]) {
	int c, i=0, a[2], map[256];
	FILE *fpin, *fpout;

	if (argc != 4) {
		fprintf(stderr,"Need exactly 3 arguments: -flag input-file output-file,"
			" got %d arguments\n", argc-1);
		return 1;
	}
	if (argv[1][0] != '-') {
		fprintf(stderr,"First argument must be an option starting with dash,"
			" got %s\n", argv[1]);
		return 2;
	}
	switch (argv[1][1]) {	// argv[1] must be option
	case 'a':
	case 'h':
		break;	// option is good
	default:
		fprintf(stderr,"Unknown option \"%s\","
			" expecting either \"-a\" or \"-h\"\n", argv[1]);
		return 3;
	}

	if ((fpin = fopen(argv[2],"rb")) == NULL) {
		fprintf(stderr,"Input-file %s cannot be opened for reading\n", argv[2]);
		return 4;
	}

	if ((fpout = fopen(argv[3],"wb")) == NULL) {
		fprintf(stderr,"Output-fil %s cannot be opened for writing\n", argv[3]);
		return 5;
	}

	if (argv[1][1] == 'h') {	// convert to hex
		while ((c = fgetc(fpin)) != EOF)
			fprintf(fpout,"%02x%s",c, ++i%40 ? "":"\r\n");
		if (i % 40) fprintf(fpout,"\n");	// beautify end of file
	} else if (argv[1][1] == 'a') {	// convert to arbitrary/ascii
		for (i=0; i<256; ++i) map[i] = 0;
		map['0'] = 0;	// was already zero, but now more obvious
		map['1'] = 1;
		map['2'] = 2;
		map['3'] = 3;
		map['4'] = 4;
		map['5'] = 5;
		map['6'] = 6;
		map['7'] = 7;
		map['8'] = 8;
		map['9'] = 9;
		map['a'] = 10;
		map['b'] = 11;
		map['c'] = 12;
		map['d'] = 13;
		map['e'] = 14;
		map['f'] = 15;
		map['A'] = 10;
		map['B'] = 11;
		map['C'] = 12;
		map['D'] = 13;
		map['E'] = 14;
		map['F'] = 15;
		i = 0;
		while ((a[i] = fgetc(fpin)) != EOF) {
			if (isspace(a[i])) continue;
			if (i > 0) fputc(16*map[a[0]]+map[a[1]],fpout);
			i = 1 - i;
		}
		if (i) {
			fprintf(stderr,"Uneven number of hex digits in input-file %s,"
				" output-file garbled\n", argv[2]);
			return 6;
		}
	}

	fclose(fpout);
	fclose(fpin);
	return 0;
}
