/* GHP MCC

   Read DB2 DSNTIAUL control and data file (in EBCDIC) 
   and convert to DB2 IMPORT control file and data file
   in ASCII method-L format.

   Synopsis:
   dsntiaul ctrlfile datafile decpfile

   ctrlfile is DSNTIAUL load control file
   datafile is actual data file in DSNTIAUL format (data in EBCDIC)
   decpfile contains info on decimal point setting: col decpnt

   Elmar Klausmeier, 17-May-2011, 16/17/20-Jun-2011
*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>	// for mmap()
#include <sys/types.h>	// for open(), fstat()
#include <sys/stat.h>	// for open(), fstat()
#include <fcntl.h>	// for open(), fstat()

extern int isblank (int c);	// gcc -Wall doesn't know about isblank()

// Check whether we are still beyond the end
#define BEYOND(N)	\
	if (p + N >= cend) {	\
		fprintf(stderr,"%s: premature end of control-file %s\n",	\
			argv[0], argv[optind]);				\
		err = 21;	\
		goto L3;	\
	}

#define DCPBEYOND	\
	if (p >= pend) {	\
		fprintf(stderr,"%s: premature end of decimal-point-file %s\n",	\
			argv[0], argv[optind+2]);			\
		err = 44;	\
		goto L1;	\
	}


// Taken from dd source code in coreutils-8.9
static char const ebcdic_to_ascii[] = {
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

struct {
	char col[32];
	int from, to;	// "from" and "to" position
	int type;	// 'E'=decimal, 'C'=char, 'T'=timestamp, 'D'=date
	int len;	// length of type
	int nullpos;	// position of NULLIF indicator, 0=not null, non-zero for nullable
	int dcp;	// # of digits after decimal point
} load[128];


int main (int argc, char *argv[]) {
	int c, cfd, dfd, pfd, debug=0, err=0, ignore=0, nrec, nlimit=INT_MAX;
	int i, j, k, lead, len, reclen=0, reclenov=0, dcp, prtpnt;
	int loadmax=0;		// # of entries in load[]
	struct stat cstat, dstat, pstat;
	char *p, *c0, *cend, *d0, *dend, *p0, *pend;
	char *t, table[64], buf[32], col[32], type[32], *fnout=NULL;
	FILE *fpout=stdout;

	while ((c = getopt(argc,argv,"din:o:r:v")) != EOF) {
		switch (c) {
		case 'd':	// debug flag
			debug = 1;
			break;
		case 'i':	// ignore file size check
			ignore = 1;
			break;
		case 'n':	// limit number of records to output
			nlimit = atoi(optarg);
			break;
		case 'o':	// output filename
			fnout = optarg;
			break;
		case 'r':	// record-length override
			reclenov = atoi(optarg);
			break;
		case 'v':	// version info
			printf("%s: IBM DB2 DNSTIAUL data converter, Elmar Klausmeier, Jun-2011, version 1.0\n",
				argv[0]);
			return 0;
		default:
			fprintf(stderr,"Unknown option flag %c\n", c);
			return 1;
		}
	}

	if (optind >= argc) {
		fprintf(stderr,"%s: missing control-file as first argument\n", argv[0]);
		return 2;
	}
	if (optind >= argc - 1) {
		fprintf(stderr,"%s: missing data-file as second argument\n", argv[0]);
		return 3;
	}
	if (optind >= argc - 2) {
		fprintf(stderr,"%s: missing decimal-point-file as first argument\n", argv[0]);
		return 4;
	}
	// cfd - control-file descriptor
	if ((cfd = open(argv[optind],O_RDONLY)) < 0) {
		fprintf(stderr,"%s: cannot open control-file %s for reading - %s\n",
			argv[0], argv[optind], strerror(errno));
		return 5;
	}
	if (fstat(cfd,&cstat) != 0) {
		fprintf(stderr,"%s: fstat() error for %s - %s\n",
			argv[0], argv[optind], strerror(errno));
		return 6;
	}
	// dfd - data-file descriptor
	if ((dfd = open(argv[optind+1],O_RDONLY)) < 0) {
		fprintf(stderr,"%s: cannot open data-file %s for reading - %s\n",
			argv[0], argv[optind+1], strerror(errno));
		return 7;
	}
	if (fstat(dfd,&dstat) != 0) {
		fprintf(stderr,"%s: fstat() error for %s - %s\n",
			argv[0], argv[optind+1], strerror(errno));
		return 8;
	}
	// pfd - decimal-point-file descriptor
	if ((pfd = open(argv[optind+2],O_RDONLY)) < 0) {
		fprintf(stderr,"%s: cannot open decimal-point-file %s for reading - %s\n",
			argv[0], argv[optind+2], strerror(errno));
		return 9;
	}
	if (fstat(pfd,&pstat) != 0) {
		fprintf(stderr,"%s: fstat() error for %s - %s\n",
			argv[0], argv[optind+2], strerror(errno));
		return 10;
	}

	if (fnout != NULL) {
		if ((fpout = fopen(fnout,"w")) == NULL) {
			fprintf(stderr,"%s: cannot open %s - %s\n",
				argv[0], fnout, strerror(errno));
			return 11;
		}
	}

	if (dstat.st_size == 0) {	// mmap() doesn't like size 0, so shortcut
		if (fpout != stdout) fclose(fpout);
		close(pfd);
		close(dfd);
		close(cfd);
		return 0;
	}

	/************************************
	 * Working on DSNTIAUL control-file *
	 ************************************/

	/* Control-file might look like this:
	  LOAD DATA LOG NO INDDN SYSREC RESUME YES INTO TABLE
	      CB.DHCEVNTB
	   (
	   HDGEKONR                               POSITION(       1         )
	   CHAR(                     16) ,
	   HDCELFNR                               POSITION(      17:      18)
	   DECIMAL                       ,
	   HDTHRESH                               POSITION(      19:      28)
	   DECIMAL
		NULLIF(      29)='?',
	   HDTRSHCY                               POSITION(      30         )
	   CHAR(                      3) ,
	   HDCREVK                                POSITION(      33         )
	   CHAR(                      3) ,
	   HDGSBVNR                               POSITION(      36         )
	   CHAR(                     16)
	   )
	*/

	// Scan DSNTIAUL control-file: use mmap() to read control-file
	c0 = mmap(0,cstat.st_size,PROT_READ,MAP_PRIVATE,cfd,0);
	if (c0 == MAP_FAILED) {
		fprintf(stderr,"%s: mmap() error for contro-file %s - %s\n",
			argv[0], argv[optind], strerror(errno));
		return 12;
	}
	if (madvise(c0,cstat.st_size,MADV_SEQUENTIAL) != 0) {
		fprintf(stderr,"%s: madvise() error for control-file %s - %s\n",
			argv[0], argv[optind], strerror(errno));
		return 13;
	}
	cend = c0 + cstat.st_size;

	// LOAD DATA LOG NO INDDN SYSREC RESUME YES INTO TABLE CB.DHGESTTB
	for (p=c0; p<cend && isspace(*p); ++p) ;	// skip leading space, if any
	BEYOND(sizeof("LOAD"))
	if (strncmp(p,"LOAD",sizeof("LOAD")-1) != 0) {
		fprintf(stderr,"%s: LOAD statement missing in control-file %s\n",
			argv[0], argv[optind]);
		err = 20;
		goto L3;
	}
	// Skip "LOAD" and other keywords until we find "INTO"
	for (p+=sizeof("LOAD"); p<cend; ++p) {
		while (p<cend && isspace(*p)) ++p;	// skip white space
		BEYOND(sizeof("INTO"))
		if (strncmp(p,"INTO",sizeof("INTO")-1) == 0) break;
	}
	p += sizeof("INTO");
	while (p<cend && isspace(*p)) ++p;	// skip white space
	BEYOND(sizeof("TABLE"))
	if (strncmp(p,"TABLE",sizeof("TABLE")-1) != 0) {
		fprintf(stderr,"%s: TABLE keyword expected in control-file %s\n",
			argv[0], argv[optind]);
		err = 22;
		goto L3;
	}
	p += sizeof("TABLE");
	while (p<cend && isspace(*p)) ++p;	// skip white space
	// Now store table name. Table name may include dot, e.g., CB.DHGESTTB
	for (t=table; p<cend && (isalnum(*p) || *p == '.'); ++p) {
		*t++ = *p;
		if (t - table >= sizeof(table)) {
			*t = '\0';
			fprintf(stderr,"%s: table name (%s) too long n control-file %s\n",
				argv[0], table, argv[optind]);
			err = 23;
			goto L3;
		}
	}
	*t = '\0';	// finish table[].
	if (debug) printf("table=|%s|\n",table);
	while (p<cend && isspace(*p)) ++p;	// skip white space
	if (*p != '(') {
		fprintf(stderr,"%s: opening bracket '(' expected in control-file %s\n",
			argv[0], argv[optind]);
		err = 24;
		goto L3;
	}

	// Work on load[], i.e., iterate thru all columns in LOAD statement
	for (loadmax=0; ++p<cend; ++loadmax) {
		if (loadmax >= sizeof(load)/sizeof(load[0])) {
			fprintf(stderr,"%s: internal array load[] exceeded for control-file %s, loadmax=%d\n",
				argv[0], argv[optind], loadmax);
			err = 25;
			goto L3;
		}

		// Initialize load[] for this index
		load[loadmax].col[0] = '\0';
		load[loadmax].from = 0;
		load[loadmax].to = 0;
		load[loadmax].type = 0;
		load[loadmax].len = 0;
		load[loadmax].nullpos = 0;	// no NULLIF seen so far
		load[loadmax].dcp = 0;

		while (p<cend && isspace(*p)) ++p;	// skip white space before column
		for (t=load[loadmax].col; p<cend && isalnum(*p); ++p) {
			*t++ = *p;	// copy column character
			if (t - load[loadmax].col >= sizeof(load[0].col)) {
				*t = '\0';
				fprintf(stderr,"%s: column (%s) too long in control-file %s\n",
					argv[0], load[loadmax].col, argv[optind]);
				err = 26;
				goto L3;
			}
		}
		*t = '\0';	// finish load[].col[]
		if (debug) printf("load[%d].col=|%s|\n", loadmax, load[loadmax].col);

		while (p<cend && isspace(*p)) ++p;	// skip white space after column
		BEYOND(sizeof("POSITION"))
		if (strncmp(p,"POSITION",sizeof("POSITION")-1) != 0) {
			fprintf(stderr,"%s: POSITION statement missing in control-file %s after column %d (%s)\n",
				argv[0], argv[optind], loadmax+1, load[loadmax].col);
			err = 27;
			goto L3;
		}
		p += sizeof("POSITION") - 1;

		while (p<cend && isspace(*p)) ++p;	// skip white space after POSITION keyword
		if (*p != '(') {
			fprintf(stderr,"%s: in control-file %s opening bracket '(' expected after POSITION keyword for column %d (%s)\n",
				argv[0], argv[optind], loadmax+1, load[loadmax].col);
			err = 28;
			goto L3;
		}
		++p;
		while (p<cend && isspace(*p)) ++p;	// skip white space after '('
		for (t=buf; p<cend && isdigit(*p); ++p) {
			*t++ = *p;	// copy from-position digit
			if (t - buf >= sizeof(buf)) {
				*t = '\0';
				fprintf(stderr,"%s: too many digits (%s) for from-position of column %d (%s) in control-file %s\n",
					argv[0], buf, loadmax+1, load[loadmax].col, argv[optind]);
				err = 29;
				goto L3;
			}
		}
		*t = '\0';	// finish load[].from
		load[loadmax].from = atoi(buf);
		if (debug) printf("load[%d].from=|%s|=%d\n", loadmax, buf, load[loadmax].from);

		while (p<cend && isspace(*p)) ++p;	// skip white space after digits in POSITION
		if (*p == ':') {
			++p;
			while (p<cend && isspace(*p)) ++p;	// skip white space after colon
			for (t=buf; p<cend && isdigit(*p); ++p) {
				*t++ = *p;	// copy to-position digit
				if (t - buf >= sizeof(buf)) {
					*t = '\0';
					fprintf(stderr,"%s: too many digits (%s) for to-position of column %d (%s) in control-file %s\n",
						argv[0], buf, loadmax+1, load[loadmax].col, argv[optind]);
					err = 30;
					goto L3;
				}
			}
			*t = '\0';	// finish load[].to
			load[loadmax].to = atoi(buf);
			if (debug) printf("load[%d].to=|%s|=%d\n", loadmax, buf, load[loadmax].to);
			while (p<cend && isspace(*p)) ++p;	// skip white space after digits in POSITION
		}
		if (*p != ')') {
			fprintf(stderr,"%s: in control-file %s closing bracket ')' expected after POSITION keyword for column %d (%s)\n",
				argv[0], argv[optind], loadmax+1, load[loadmax].col);
			err = 31;
			goto L3;
		}

		// Determine type of column
		++p;
		while (p<cend && isspace(*p)) ++p;	// skip white space after ')'
		if (p+sizeof("CHAR")<cend && strncmp(p,"CHAR",sizeof("CHAR")-1) == 0) {
			load[loadmax].type = 'C';
			p += sizeof("CHAR") - 1;
		} else if (p+sizeof("DECIMAL")<cend && strncmp(p,"DECIMAL",sizeof("DECIMAL")-1) == 0) {
			load[loadmax].type ='E';
			p += sizeof("DECIMAL") - 1;
		} else if (p+sizeof("TIMESTAMP EXTERNAL")<cend && strncmp(p,"TIMESTAMP EXTERNAL",sizeof("TIMESTAMP EXTERNAL")-1) == 0) {
			load[loadmax].type ='S';
			p += sizeof("TIMESTAMP EXTERNAL") - 1;
		} else if (p+sizeof("TIME EXTERNAL")<cend && strncmp(p,"TIME EXTERNAL",sizeof("TIME EXTERNAL")-1) == 0) {
			load[loadmax].type ='T';
			p += sizeof("TIME EXTERNAL") - 1;
		} else if (p+sizeof("DATE EXTERNAL")<cend && strncmp(p,"DATE EXTERNAL",sizeof("DATE EXTERNAL")-1) == 0) {
			load[loadmax].type ='D';
			p += sizeof("DATE EXTERNAL") - 1;
		} else {
			fprintf(stderr,"%s: expect type information for column %d (%s) in control-file %s\n",
				argv[0], loadmax+1, load[loadmax].col, argv[optind]);
			err = 32;
			goto L3;
		}
		if (debug) printf("load[%d].type=%c\n", loadmax, load[loadmax].type);

		while (p<cend && isspace(*p)) ++p;	// skip white space after type
		if (load[loadmax].type != 'E') {	// non-decimal type has length
			if (*p != '(') {
				fprintf(stderr,"%s: in control-file %s opening bracket '(' expected after type for column %d (%s)\n",
					argv[0], argv[optind], loadmax+1, load[loadmax].col);
				err = 33;
				goto L3;
			}

			++p;
			while (p<cend && isspace(*p)) ++p;	// skip white space after '('
			for (t=buf; p<cend && isdigit(*p); ++p) {
				*t++ = *p;	// copy length digit
				if (t - buf >= sizeof(buf)) {
					*t = '\0';
					fprintf(stderr,"%s: too many digits (%s) for type of column %d (%s) in control-file %s\n",
						argv[0], buf, loadmax+1, load[loadmax].col, argv[optind]);
					err = 34;
					goto L3;
				}
			}
			*t = '\0';	// finish load[].len
			load[loadmax].len = atoi(buf);
			if (debug) printf("load[%d].len=|%s|=%d\n", loadmax, buf, load[loadmax].len);

			while (p<cend && isspace(*p)) ++p;	// skip white space after digits in type
			if (*p != ')') {
				fprintf(stderr,"%s: in control-file %s closing bracket ')' expected after type for column %d (%s)\n",
					argv[0], argv[optind], loadmax+1, load[loadmax].col);
				err = 35;
				goto L3;
			}
			++p;
			while (p<cend && isspace(*p)) ++p;	// skip white space after ')'
		}

		// Check for optional NULLIF
		if (p+sizeof("NULLIF")<cend && strncmp(p,"NULLIF",sizeof("NULLIF")-1) == 0) {
			p += sizeof("NULLIF") - 1;
			while (p<cend && isspace(*p)) ++p;	// skip white space after NULLIF
			if (*p != '(') {
				fprintf(stderr,"%s: in control-file %s opening bracket '(' expected after NULLIF for column %d (%s)\n",
					argv[0], argv[optind], loadmax+1, load[loadmax].col);
				err = 36;
				goto L3;
			}
			++p;
			while (p<cend && isspace(*p)) ++p;	// skip white space after '('
			for (t=buf; p<cend && isdigit(*p); ++p) {
				*t++ = *p;	// copy length digit
				if (t - buf >= sizeof(buf)) {
					*t = '\0';
					fprintf(stderr,"%s: too many digits (%s) for NULLIF of column %d (%s) in control-file %s\n",
						argv[0], buf, loadmax+1, load[loadmax].col, argv[optind]);
					err = 37;
					goto L3;
				}
			}
			*t = '\0';	// finish load[].nullpos
			load[loadmax].nullpos = atoi(buf);
			if (debug) printf("load[%d].nullpos=|%s|=%d\n", loadmax, buf, load[loadmax].nullpos);

			while (p<cend && isspace(*p)) ++p;	// skip white space after digits in type
			if (*p != ')') {
				fprintf(stderr,"%s: in control-file %s closing bracket ')' expected after NULLIF for column %d (%s)\n",
					argv[0], argv[optind], loadmax+1, load[loadmax].col);
				err = 38;
				goto L3;
			}
			++p;
			while (p<cend && isspace(*p)) ++p;	// skip white space after ')'
			if (p+sizeof("='?'")<cend && strncmp(p,"='?'",sizeof("='?'")-1) != 0) {
				fprintf(stderr,"%s: missing expression \"='?'\" in for column %d (%s) in control-file %s\n",
					argv[0], loadmax+1, load[loadmax].col, argv[optind]);
				err = 39;
				goto L3;
			}
			p += sizeof("='?'") - 1;
			while (p<cend && isspace(*p)) ++p;	// skip white space after ='?'
		}

		BEYOND(0)
		if (*p == ')') {
			break;	// reached end of all columns
		} else if (*p != ',') {
			fprintf(stderr,"%s: expecting comma for column %d (%s) in control-file %s\n",
				argv[0], loadmax+1, load[loadmax].col, argv[optind]);
			err = 40;
			goto L3;
		}
	}
	// Puh, this was lots of code just for parsing
	// We are done with the control-file, so unmap and close it
	if (munmap(c0,cstat.st_size)) {
		fprintf(stderr,"%s: munmap() error for %s\n", argv[0], argv[optind]);
		err = 41;
		goto L5;
	}
	if (close(cfd) != 0) {
		fprintf(stderr,"%s: cannot close control-file %s - %s\n",
			argv[0], argv[optind], strerror(errno));
		err = 42;
		goto L5;
	}

	if (reclenov != 0) reclen = reclenov;	// record-length given by user
	else if (load[loadmax].nullpos != 0) reclen = load[loadmax].nullpos;	// last nullpos
	else if (load[loadmax].to != 0) reclen = load[loadmax].to;	// last to
	else if (load[loadmax].len != 0) reclen = load[loadmax].from + (load[loadmax].len - 1);
	else reclen = load[loadmax].from;
	if (debug) printf("reclen=%d, dstat.st_size=%ld\n", reclen, (long)dstat.st_size);
	if (dstat.st_size % reclen != 0) {	// data-file size must be a multiple of reclen
		fprintf(stderr,"%s: specified record-length (%d) of control-file %s incommensurable to file-size (%ld) of data-file %s\n",
			argv[0], reclen, argv[optind], (long)dstat.st_size, argv[optind+1]);
		err = 43;
		goto L5;
	}


	/************************************
	 * Working on decimal-point-file    *
	 ************************************/

	/*
	table     col            # type        len    dcp rest
	DHWKHDTB  HDGEKONR       0 CHAR         16      0 Geschaefts-Komponenten-Nummer
	DHWKHDTB  DHVALGDT       1 DATE          4      0 Valuta-Datum Geldbuchung
	DHWKHDTB  HDKRSEIN       2 DECIMAL       5      0 Kurs-Einheit
	DHWKHDTB  HDAKRSWH       3 CHAR          3      0 Ausfuehrungskurs-Waehrung
	DHWKHDTB  HDAKSART       4 CHAR          1      0 Aufuehrungs-Kurs-Art
	DHWKHDTB  HDAMBTWH       5 CHAR          3      0 Waehrung Ausmachender Betrag
	DHWKHDTB  HDBUCHDT       6 DATE          4      0 Buchungs-Datum Geschaeftskompo
	DHWKHDTB  HDDECKJN       7 CHAR          1      0 Deckungs-Kennzeichen-Ja-Nein B
	DHWKHDTB  HDAKREIN       8 DECIMAL       5      0 Ausfuehrungskurs-Einheit
	*/

	// Read decimal-point-file: use mmap()
	p0 = mmap(0,pstat.st_size,PROT_READ,MAP_PRIVATE,pfd,0);
	if (p0 == MAP_FAILED) {
		fprintf(stderr,"%s: mmap() error for %s - %s\n",
			argv[0], argv[optind+2], strerror(errno));
		err = 45;
		goto L2;
	}
	if (madvise(p0,pstat.st_size,MADV_SEQUENTIAL) != 0) {
		fprintf(stderr,"%s: madvise() error - %s\n", argv[0], strerror(errno));
		err = 46;
		goto L2;
	}
	pend = p0 + pstat.st_size;

	for (p=p0; p<pend; ++p) {
		while (p<pend && isblank(*p)) ++p;	// skip blanks before table-name
		DCPBEYOND
		while (p<pend && isalnum(*p)) ++p;	// eat table-name
		DCPBEYOND
		while (p<pend && isblank(*p)) ++p;	// skip blanks before column-name
		DCPBEYOND
		for (t=col; p<pend && isalnum(*p); ++p) {
			*t++ = *p;	// copy column-name
			if (t - col >= sizeof(col)) {
				*t = '\0';
				fprintf(stderr,"%s: too many chars (%s) for column in decimal-point-file %s\n",
					argv[0], col, argv[optind+2]);
				err = 47;
				goto L1;
			}
		}
		*t = '\0';	// finish col[]
		if (debug) printf("col=|%s|\n", col);
		DCPBEYOND
		while (p<pend && isblank(*p)) ++p;	// skip blanks before column-number
		DCPBEYOND
		while (p<pend && isdigit(*p)) ++p;	// eat column-number
		DCPBEYOND
		while (p<pend && isblank(*p)) ++p;	// skip blanks before type
		DCPBEYOND
		for (t=type; p<pend && isalnum(*p); ++p) {
			*t++ = *p;	// copy type-info
			if (t - type >= sizeof(type)) {
				*t = '\0';
				fprintf(stderr,"%s: too many chars (%s) for type in decimal-point-file %s\n",
					argv[0], col, argv[optind+2]);
				err = 48;
				goto L1;
			}
		}
		*t = '\0';	// finish type[]
		if (debug) printf("type=|%s|\n", type);
		DCPBEYOND
		while (p<pend && isblank(*p)) ++p;	// skip blanks before length
		DCPBEYOND
		while (p<pend && isdigit(*p)) ++p;	// eat length
		DCPBEYOND
		while (p<pend && isblank(*p)) ++p;	// skip blanks before dcp
		DCPBEYOND
		for (t=buf; p<pend && isdigit(*p); ++p) {
			*t++ = *p;	// copy type-info
			if (t - buf >= sizeof(buf)) {
				*t = '\0';
				fprintf(stderr,"%s: too many chars (%s) for dcp in decimal-point-file %s\n",
					argv[0], buf, argv[optind+2]);
				err = 49;
				goto L1;
			}
		}
		*t = '\0';	// finish dcp
		dcp = atoi(buf);
		if (debug) printf("dcp=|%s|=%d\n", buf, dcp);
		DCPBEYOND
		while (p<pend && *p != '\n') ++p;	// reach end-of-line

		// Now search col in load[] if type is DECIMAL by conducting a linear search
		if (strcmp(type,"DECIMAL") == 0) {
			if (debug) printf("Searching for %s\n", col);
			for (i=0; i<=loadmax; ++i) {
				if (strcmp(load[i].col,col) == 0) {
					if (load[i].type != 'E') {
						fprintf(stderr,"%s: type mismatch in control-file %s and decimal-point-file %s for column %s\n",
							argv[0], argv[optind], argv[optind+2], col);
						err = 50;
						goto L1;
					}
					load[i].dcp = dcp;
					if (debug) printf("load[%d].dcp=%d\n", i, dcp);
					break;
				}
			}
		}
	}


	/************************************
	 * Working on DSNTIAUL data-file    *
	 ************************************/

	// Read DSNTIAUL data-file: use mmap()
	d0 = mmap(0,dstat.st_size,PROT_READ,MAP_PRIVATE,dfd,0);
	if (d0 == MAP_FAILED) {
		fprintf(stderr,"%s: mmap() error for data-file %s - %s\n",
			argv[0], argv[optind+1], strerror(errno));
		err = 51;
		goto L5;
	}
	if (madvise(d0,dstat.st_size,MADV_SEQUENTIAL) != 0) {
		fprintf(stderr,"%s: madvise() error for data-file %s - %s\n",
			argv[0], argv[optind+1], strerror(errno));
		err = 52;
		goto L5;
	}
	dend = d0 + dstat.st_size;

	if (debug) {
		for (i=0; i<=loadmax; ++i)
			printf("col=%s from=%d to=%d type=%c len=%d nullpos=%d dcp=%d\n",
				load[i].col, load[i].from, load[i].to,
				load[i].type, load[i].len, load[i].nullpos,
				load[i].dcp);
	}
	nrec = 0;
	for (p=d0; p<dend; p+=reclen) {
//printf("p=%ld, *p=%c=%x -> %c\n", (long)p, *p, *p, ebcdic_to_ascii[*p & 0xFF]);
		if (++nrec > nlimit) break;	// user-limit exceeded
		for (i=0; i<=loadmax; ++i) {
			if (i != 0) fputc(',',fpout);
			if (load[i].nullpos == 0
			||  ebcdic_to_ascii[ p[load[i].nullpos-1] & 0xFF ] != '?') {
				t = p + load[i].from - 1;
				switch (load[i].type) {
				case 'C':	// CHAR
				case 'D':	// DATE EXTERNAL
				case 'S':	// TIMESTAMP EXTERNAL
				case 'T':	// TIME EXTERNAL
					// Convert character data from EBCDIC to ASCII
					fputc('"',fpout);
					for (j=0; j<load[i].len; ++j)
						fputc( ebcdic_to_ascii[t[j] & 0xFF], fpout );
					fputc('"',fpout);
					break;
				case 'E':	// DECIMAL
					// Convert packed decimal to ASCII string
					// This is probably the most important part of the program,
					// although basically simple.
					lead = 1;
					len = load[i].to - load[i].from;	// this is (length - 1)
					prtpnt = 2 * (len + 1) - 1 - load[i].dcp;
					k = 0;
					if ((t[len] & 0x0F) == 0x0D)	// C=Credit=+, D=Debit=-
						fputc('-',fpout);
					for (j=0; j<len; ++j) {
						c = t[j] & 0xFF;
						if (lead == 0  ||  c != 0x00) {	// skip leading zeros
							if (k++ == prtpnt) fputs(lead<0? "0." : ".",fpout);
							if (lead == 0  ||  (c & 0xF0) != 0x00)
								fputc('0' + ((c & 0xF0) >> 4),fpout);
							if (k++ == prtpnt) fputs(lead<0? "0." : ".",fpout);
							fputc('0' + (c & 0x0F),fpout);
							lead = 0;	// zeros are now in the middle of the number
						} else {
							lead = -1;	// there is at least one leading zero
							if (k++ == prtpnt) {
								lead = 0;
								fputs("0.",fpout);	// 0 from previous byte
							} else if (k++ == prtpnt) {
								lead = 0;
								fputs("0.0",fpout);	// 0 from previous byte + actual 0
							}
						}
					}
					// Output last digit
					if (k++ == prtpnt) fputs(lead<0? "0." : ".",fpout);
					fputc('0' + ((t[len] & 0xF0) >> 4),fpout);
					break;
				default:
					fprintf(stderr,"%s: unknown type %x, internal program error\n",
						argv[0], load[i].type);
					err = 53;
					goto L4;
				}
			}
		}
		fputc('\n',fpout);
	}

	// Unmap memory, close data-file and output-file
	if (munmap(d0,dstat.st_size)) perror(argv[optind+1]);
	if (close(dfd) != 0) perror(argv[optind+1]);
	if (fpout != stdout  &&  fclose(fpout) != 0) perror(fnout);

	return 0;

L1:
	munmap(p0,pstat.st_size);
L2:
	close(pfd);
L3:
	munmap(c0,cstat.st_size);
	close(cfd);
	close(dfd);
	return err;
L4:
	munmap(d0,dstat.st_size);
L5:
	close(dfd);
	return err;
}

