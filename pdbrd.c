/* Read Palm PDB/PC3 files

   Record description is from
	Palm File Format Specification,
	Written by Gary Hillerson,
	contributions by Kenneth Albanowski, John Marshall, Keith Rollin
	May 1, 2001
   Found this book at
	https://urchin.earth.li/darcs/ganesh/gps/Palm_File_Format_Specs.pdf

   Elmar Klausmeier, 17-Feb-2013
*/


#include <stdio.h>
#include <errno.h>	// for errno
#include <string.h>	// for strerror()
#include <stdlib.h>	// for atoi()
#include <sys/mman.h>	// for mmap()
#include <sys/types.h>	// for open(), fstat()
#include <sys/stat.h>	// for open(), fstat()
#include <fcntl.h>	// for open(), fstat()
#include <arpa/inet.h>	// for ntohl()
#include <unistd.h>	// for getopt()
#include <time.h>	// for ctime()
#include <limits.h>	// for INT_MAX

// This is from pilot-link: libpisock/pi-file.c
#define PILOT_TIME_DELTA (unsigned)(2082844800)


typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef unsigned int UInt32;
typedef unsigned int LocalID;

typedef struct {	// header of record list
	LocalID nextRecordListID;
	UInt16 numRecords;
	UInt16 firstEntry;
} RecordListType;

typedef struct {	// single element of record list
	LocalID localChunkID;	// offset, start of actual data
	UInt8 attributes;	// 1st nibble: ?/private, 2nd: this is the category
	UInt8 uniqueID[3];
} RecordEntryType;

typedef struct {
	UInt16 renamedCategories;
	char categoryLabels[16][16];
	UInt8 categoryUniqIDs[16];
	UInt16 lastUniqID_pad;	//UInt8 lastUniqID; UInt8 padding;
} AppInfoType;

typedef struct {
	UInt8 name[32];
	UInt16 attributes;
	UInt16 version;		// not fully contained in Palm document
	UInt32 creationDate;
	UInt32 modificationDate;
	UInt32 lastBackupDate;
	UInt32 modificationNumber;
	LocalID appInfoID;
	LocalID sortInfoID;
	UInt32 type;
	UInt32 creator;
	UInt32 uniqueIDSeed;
	RecordListType recordList;
} DatabaseHdrType;

typedef struct {
	UInt32 phoneLabel;	// array of 8 nibbles
	UInt32 contents;	// bit mask
} AddressRecordHeader;

typedef struct {
	UInt32 labelRenamed;
	UInt8 labels[22][16];
	UInt16 country;
	UInt16 sortByCompany;
} AddressAppInfo;

// This is from jpilot-1.8.1/libplugin.h
// This bit means that this record is of no importance anymore
#define SPENT_PC_RECORD_BIT 256

typedef enum {
	PALM_REC = 100L,
	MODIFIED_PALM_REC = 101L,	// this is the "old" record
	DELETED_PALM_REC = 102L,
	NEW_PC_REC = 103L,		// completely new
	DELETED_PC_REC =  SPENT_PC_RECORD_BIT | 104L,
	DELETED_DELETED_PALM_REC =  SPENT_PC_RECORD_BIT | 105L,
	REPLACEMENT_PALM_REC = 106L	// this is the new record, replacing something old
} PCRecType;

typedef struct {
	UInt32 header_len;	// always 21
	UInt32 header_version;	// always 2
	UInt32 rec_len;
	UInt32 unique_id;
	UInt32 rt;	// record type, see PCRecType above
	UInt8 attrib;	// 2 nibbles: private/category
} PC3RecordHeader;


static int debug = 0;
static int gdata = 0;


//#define HOFFSET(x)	((void*)&hdr.x - (void*)&hdr)
#define HOFFSET(x)	((void*)&hdr->x - (void*)hdr)
#define hi(x)		(((x) >> 4) & 0x0F)
#define lo(x)		((x) & 0x0F)




/* p0 points to complete PDB file.
   After this call hdr is populated with all header information.
*/
int readPDBHeader (const char *p0, DatabaseHdrType *hdr) {
	DatabaseHdrType *h;
	char buf1[26], buf2[26], buf3[26], type[5], creator[5];
	time_t creationDate, modificationDate, lastBackupDate;

	h = (DatabaseHdrType*)p0;	// now h contains complete header

	memcpy(hdr->name,h->name,32);
	hdr->attributes = ntohl(h->attributes);
	hdr->version = ntohl(h->version);
	hdr->creationDate = ntohl(h->creationDate);
	hdr->modificationDate = ntohl(h->modificationDate);
	hdr->lastBackupDate = ntohl(h->lastBackupDate);
	creationDate = hdr->creationDate - PILOT_TIME_DELTA;
	modificationDate = hdr->modificationDate - PILOT_TIME_DELTA;
	lastBackupDate = hdr->lastBackupDate - PILOT_TIME_DELTA;
	hdr->modificationNumber = ntohl(h->modificationNumber);
	hdr->appInfoID = ntohl(h->appInfoID);
	hdr->sortInfoID = ntohl(h->sortInfoID);
	hdr->type = ntohl(h->type);	// 4 byte char string without '\0'
	hdr->creator = ntohl(h->creator);// 4 byte char string without '\0'
	type[0] = p0[0x3c];
	type[1] = p0[0x3d];
	type[2] = p0[0x3e];
	type[3] = p0[0x3f];
	type[4] = '\0';
	creator[0] = p0[0x40];
	creator[1] = p0[0x41];
	creator[2] = p0[0x42];
	creator[3] = p0[0x43];
	creator[4] = '\0';
	hdr->uniqueIDSeed = ntohl(h->uniqueIDSeed);
	hdr->recordList.nextRecordListID = ntohl(h->recordList.nextRecordListID);
	hdr->recordList.numRecords = ntohs(h->recordList.numRecords);
	hdr->recordList.firstEntry = ntohs(h->recordList.firstEntry);

	if (gdata == 0) {
		printf("Header, size=%ld=0x%0lx\n"
			"\tname=%s\n"
			"\tattributes=0x%0x\n"
			"\tversion=0x%0x\n"
			"\tcreationDate=%s"
			"\tmodificationDate=%s"
			"\tlastBackupDate=%s"
			"\tmodificationNumber=%u\n"
			"\tappInfoID=%u=0x%0x\n"
			"\t%lx sortInfoID=%u\n"
			"\t%lx type=0x%0x=%s\n"
			"\t%lx creator=0x%0x=%s\n"
			"\t%lx uniqueIDSeed=%u\n"
			"\t%lx recordList.nextRecordListID=%u\n"
			"\t%lx recordList.numRecords=%u\n"
			"\t%lx recordList.firstEntry=%u\n",
			(long)sizeof(*hdr), (long)sizeof(*hdr),
			h->name, hdr->attributes, hdr->version,
			ctime_r(&creationDate,buf1),
			ctime_r(&modificationDate,buf2),
			ctime_r(&lastBackupDate,buf3),
			hdr->modificationNumber,
			hdr->appInfoID, hdr->appInfoID,
			HOFFSET(sortInfoID), hdr->sortInfoID,
			HOFFSET(type), hdr->type, type,
			HOFFSET(creator), hdr->creator, creator,
			HOFFSET(uniqueIDSeed), hdr->uniqueIDSeed,
			HOFFSET(recordList.nextRecordListID), hdr->recordList.nextRecordListID,
			HOFFSET(recordList.numRecords), (unsigned)hdr->recordList.numRecords,
			HOFFSET(recordList.firstEntry), (unsigned)hdr->recordList.firstEntry);
	}

	return 0;
}



/* Read AddressRecordHeader and corresponding data.
   This will show actual address data for a single record.
*/
int readAddrRecHdr (const char *p1, const unsigned char *phoneType[8]) {
	int k;
	AddressRecordHeader *arh;
	unsigned contents;
	unsigned int phoneLabel;
	int showPhone;
	int phoneLabelArr[5];
	const char *s;
	char *label[22] = {
		"Last name", "First name", "Company",
		"Work", "Home", "Fax", "Other", "E-mail",
		"Addr(W)", "City", "State", "Zip Code", "Country",
		"Title", "UserId", "Custom 2", "Birthday", "Custom 4",
		"Note", "Main", "Pager", "Mobile"
	};


	arh = (AddressRecordHeader*)(p1);
	contents = ntohl(arh->contents);
	// This is from unpack_Address() in libpisock/address.c
	showPhone        = hi(p1[1]);
	phoneLabelArr[4] = lo(p1[1]);
	phoneLabelArr[3] = hi(p1[2]);
	phoneLabelArr[2] = lo(p1[2]);
	phoneLabelArr[1] = hi(p1[3]);
	phoneLabelArr[0] = lo(p1[3]);
	phoneLabel = arh->phoneLabel;
	if (gdata == 0) {
		printf("\tphoneLabel=0x%0x (show=%d,%d,%d,%d,%d,%d), contents=0x%0x\n",
			phoneLabel,
			showPhone, phoneLabelArr[0],phoneLabelArr[1],phoneLabelArr[2],phoneLabelArr[3],phoneLabelArr[4],
			contents);
	}

	s = p1 + 9;
	for (k=0; k<19; ++k) {
		if ((contents & (0x01 << k)) != 0) {
			if (k >= 3 && k<=7) {
				if (gdata == 0) {
					printf("{%d:%s%s}",
						k,
						(k - 3 == showPhone) ? "*" : "",
						phoneType[phoneLabelArr[k-3] & 0x07]);
				} else if (gdata == 1) {
					printf(",(\"%s\",\"",
						phoneType[phoneLabelArr[k-3] & 0x07]);
				}
			} else {
				if (gdata == 0) {
					printf("{%d}",k);
				} else if (gdata == 1) {
					printf(",(\"%s\",\"",label[k]);
				}
			}
			if (gdata == 0) {
				while (*s) putchar(*s++);
			} else if (gdata == 1) {
				while (*s) {
					if (*s == '\'' || *s == '"') putchar('\\');
					if (*s == '\t') { putchar('\\'); putchar('t'); ++s; }
					else if (*s == '\n') { putchar('\\'); putchar('n'); ++s; }
					else if (*s == '\\') { putchar('\\'); putchar('\\'); ++s; }
					else putchar(*s++);
				}
			}
			++s;
			if (gdata == 0) putchar('|');
			else if (gdata == 1) printf("\")");
		}
	}
	if (gdata == 0) puts("");
	else if (gdata == 1) putchar(']');

	return 0;
}



/* p0 is pointer to PDB, hdr is header,
   nrecords is maximum number of records to process.
   Read all addresses.
*/
int readPDBAddr (const char *p0, const DatabaseHdrType *hdr, int nrecords) {
	int i;
	RecordEntryType *r;
	int localChunkID;
	AppInfoType *ap;
	AddressAppInfo *addAI;
	const unsigned char *phoneType[8];

	if (hdr->appInfoID != 0) {
		ap = (AppInfoType*)(p0 + hdr->appInfoID);

		if (gdata == 0) {
			printf("applInfo, size=%ld=0x%0lx\n"
				"\trenamedCategories=%u\n",
				sizeof(AppInfoType), sizeof(AppInfoType),
				ap->renamedCategories);
			for (i=0; i<16; ++i)
				printf("\tcategoryUniqIDs[%2d]=%-4u    categoryLabels[%2d]=%s\n",
					i, ap->categoryUniqIDs[i], i, ap->categoryLabels[i]);
			printf("\tlastUniqID_pad=%u\n", ap->lastUniqID_pad);
				//"\tpadding=%u\n",ap->lastUniqID, ap->padding
		}
	} else return 1;

	//addAI = (AddressAppInfo*)(p0 + hdr->appInfoID + sizeof(AppInfoType) + 2);
	addAI = (AddressAppInfo*)((char*)ap + sizeof(AppInfoType) + 2);
	// This is from pilot-link: unpack_AddressAppInfo() in libpisock/address.c
	phoneType[0] = addAI->labels[3];
	phoneType[1] = addAI->labels[4];
	phoneType[2] = addAI->labels[5];
	phoneType[3] = addAI->labels[6];
	phoneType[4] = addAI->labels[7];
	phoneType[5] = addAI->labels[19];
	phoneType[6] = addAI->labels[20];
	phoneType[7] = addAI->labels[21];
	if (gdata == 0) {
		printf("Address Application Info at 0x%0x / 0x%0x:\n"
			"\tlabelRenamed=0x%0x\n",
			hdr->appInfoID + ((int)sizeof(AppInfoType)) + 2,
			(unsigned int)((char*)addAI - (char*)p0),
			ntohl(addAI->labelRenamed));
		for (i=0; i<22; ++i) {
			if (i < 8)
				printf("\tlabels[%2d]=%-16s\tphoneType[%d]=%s\n",
					i, addAI->labels[i], i, phoneType[i]);
			else
				printf("\tlabels[%2d]=%-16s\n", i, addAI->labels[i]);
		}
		printf("\tcountry=%u, sortByCompany=%u\n",
			addAI->country, addAI->sortByCompany);
	} else if (gdata == 1) {
		puts("list = [");
	}

	for (i=0; i<nrecords; ++i) {
		if (gdata == 1 && i > 0) puts(",");
		r = (RecordEntryType*)(p0 + 0x04e + i*8);
		localChunkID = ntohl(r->localChunkID);
		if (gdata == 0) {
			printf("%d Record Entry:\n"
				"\tlocalChunkID=0x%0x=%s\n"
				"\tattributes/category=%d=0x%0x/%s, uniqueID=%d\n",
				i+1, localChunkID, p0+localChunkID+9,
				(int)r->attributes, (int)r->attributes,
				ap->categoryLabels[r->attributes & 0x0F],
				r->uniqueID[0]*0x010000 + r->uniqueID[1]*0x0100 + r->uniqueID[2]);
		} else if (gdata == 1) {
			printf("\t[(\"uniqueID\",\"%d\")",
				r->uniqueID[0]*0x010000 + r->uniqueID[1]*0x0100 + r->uniqueID[2]);
		}

		readAddrRecHdr(p0+localChunkID,phoneType);
	}

	if (gdata == 1) puts("\n]");

	return 0;
}


int readPC3Addr (const char *p0, int st_size, int nrecords) {
	int i, rec_len, offset=0;
	PC3RecordHeader *pc3rh;
	const char *phoneType[8] = { "Work","Home","Fax","Other","E-mail","Main","Pager","Mobile" };

	for (i=0; i<nrecords && offset<st_size; ++i) {
		if (gdata == 1 && i > 0) puts(",");
		pc3rh = (PC3RecordHeader*) p0;
		rec_len = ntohl(pc3rh->rec_len);
		if (gdata == 0) {
			printf("%d PC3 Record Header:\n"
				"\theader_len=%d, "
				"\theader_version=%d, "
				"\trec_len=%d\n"
				"\tunique_id=%d, "
				"\trt=%d, "
				"\tattrib=0x%02x\n",
				i+1,
				ntohl(pc3rh->header_len),
				ntohl(pc3rh->header_version),
				rec_len,
				ntohl(pc3rh->unique_id),
				ntohl(pc3rh->rt),
				pc3rh->attrib);
		} else if (gdata == 1) {
			printf("\t[(\"uniqueID\",\"%d\")",
				ntohl(pc3rh->unique_id));
		}

		readAddrRecHdr(p0+21,(const unsigned char**)phoneType);

		offset += 21 + rec_len;	// 21 is sizeof(PC3RecordHeader)
		p0 += 21 + rec_len;
	}

	if (gdata == 1) puts("\n]");

	return 0;
}



int main (int argc, char *argv[]) {
	int c, nrecords=0, pc3=0, pfd, ret=0;
	struct stat pstat;
	char *p0;
	DatabaseHdrType hdr;


	while ((c = getopt(argc,argv,"cdgn:")) != EOF) {
		switch (c) {
		case 'c':
			pc3 = 1;
			break;
		case 'd':	// debug flag
			debug = 1;
			break;
		case 'g':
			gdata = 1;
			break;
		case 'n':
			// nrecords=0 means: all records
			nrecords = atoi(optarg);
			break;
		default:
			fprintf(stderr,"Unknown option flag %c\n", c);
			return 1;
		}
	}

	if (optind >= argc) {
		fprintf(stderr,"%s: missing file name\n", argv[0]);
		return 2;
	}

	if ((pfd = open(argv[optind],O_RDONLY)) < 0) {
		fprintf(stderr,"%s: cannot open file %s for reading - %s\n",
			argv[0], argv[optind], strerror(errno));
		return 3;
	}
	if (fstat(pfd,&pstat) != 0) {
		fprintf(stderr,"%s: fstat() error for %s - %s\n",
			argv[0], argv[optind], strerror(errno));
		return 4;
	}

	if (pstat.st_size == 0) {	// mmap() doesn't like size 0, so shortcut
		close(pfd);
		return 0;
	}
	if (gdata == 0) printf("pstat.st_size=%ld\n",(long)pstat.st_size);

	p0 = mmap(0,pstat.st_size,PROT_READ,MAP_PRIVATE,pfd,0);
	if (p0 == MAP_FAILED) {
		fprintf(stderr,"%s: mmap() error for contro-file %s - %s\n",
			argv[0], argv[optind], strerror(errno));
		return 5;
	}

	if (pc3 == 1) {	// PC3
		if (pstat.st_size <= 21) {
			printf("Cannot read PC3RecordHeader of 21 bytes, st_size=%ld\n",pstat.st_size);
			ret = 6;
		} else
			readPC3Addr(p0,pstat.st_size,nrecords ? nrecords : INT_MAX);
	} else {	// PDB
		readPDBHeader(p0,&hdr);

		// nrecords=0 means: all records
		if (nrecords == 0  ||  nrecords > hdr.recordList.numRecords)
			nrecords = hdr.recordList.numRecords;


		if (pstat.st_size <= 0x058) {
			printf("No record list after header, st_size=%ld\n",pstat.st_size);
			ret = 6;
		} else if (hdr.appInfoID >= pstat.st_size) {
			printf("appInfoID=%u exceeds st_size=%ld\n",
				hdr.appInfoID,(long)pstat.st_size);
			ret = 7;
		} else if (hdr.appInfoID == 0) {
			puts("hdr.appInfoID == 0");
			ret = 8;
		} else if (strncmp((const char*)hdr.name,"AddressDB",32) == 0)
			readPDBAddr(p0,&hdr,nrecords);
	}

//finish:
	munmap(p0,pstat.st_size);
	close(pfd);

	return ret;
}

/*
	//type[0] = (hdr->type >> 24) & 0xFF;
	//type[1] = (hdr->type >> 16) & 0xFF;
	//type[2] = (hdr->type >> 8) & 0xFF;
	//type[3] = hdr->type & 0xFF;

	//creator[0] = (hdr->creator >> 24) & 0xFF;
	//creator[1] = (hdr->creator >> 16) & 0xFF;
	//creator[2] = (hdr->creator >> 8) & 0xFF;
	//creator[3] = hdr->creator & 0xFF;


	AddressRecordHeader *arh;
	unsigned contents;
	unsigned int phoneLabel;
	int showPhone;
	int phoneLabelArr[5];
	const char *s;

		arh = (AddressRecordHeader*)(p0 + localChunkID);
		contents = ntohl(arh->contents);
		// This is from unpack_Address() in libpisock/address.c
		showPhone        = hi(p0[localChunkID+1]);
		phoneLabelArr[4] = lo(p0[localChunkID+1]);
		phoneLabelArr[3] = hi(p0[localChunkID+2]);
		phoneLabelArr[2] = lo(p0[localChunkID+2]);
		phoneLabelArr[1] = hi(p0[localChunkID+3]);
		phoneLabelArr[0] = lo(p0[localChunkID+3]);
		//phoneLabel = ntohl(arh->phoneLabel);
		phoneLabel = arh->phoneLabel;
		printf("\tphoneLabel=0x%0x (show=%d,%d,%d,%d,%d,%d), contents=0x%0x\n",
			phoneLabel,
			showPhone, phoneLabelArr[0],phoneLabelArr[1],phoneLabelArr[2],phoneLabelArr[3],phoneLabelArr[4],
			contents);

		s = p0 + localChunkID + 9;
		for (k=0; k<19; ++k) {
			if ((contents & (0x01 << k)) != 0) {
				if (k >= 3 && k<=7) {
					printf("{%d:%s%s}",
						k,
						(k - 3 == showPhone) ? "*" : "",
						phoneType[phoneLabelArr[k-3] & 0x07]);
				} else {
					printf("{%d}",k);
				}
				while (*s) putchar(*s++);
				++s;
				putchar('|');
			}
		}
		puts("");
*/

