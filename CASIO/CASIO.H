#define DEBUG		1
#define LEVEL0		0
#define LEVEL1		1
#define LEVEL2		0
#define LEVEL3		1

#define OUTFILE	1 
/* #define LOGWRITE	1  */
/* #define SIM		1  */

#define	SERIAL		1
#define TAST		2
#define PORT		SERIAL

#define CAR_RET		0x0D
#define LIN_FED		0x0A
#define BLKSTART	0x3A

/* Typus der Uebertragenen Daten */
#define UNKNOWN		0x0
#define TEXT		0x8
#define ALARM		0xC
#define DATE		0xF
#define TIME		0xE
#define MARKED		0x8
#define UNMARKED	0x0

/* Steuerbytes */
#define NOCTRL		0x00
#define SETEND		0x01
#define MODECHG		0x02
#define TRANSEND	0xFF

/* Moduswechsel-Kennungen */
#define PHONE		0x9000
#define MEMO		0xA000
#define REMIND		0x9100
#define SCHED		0xB000
#define CALEND		0x8000

/* Zust"ande beim Senden */
#define TRYING		0

/* Zust"ande beim Empfang */
#define UNDEF		-1
#define	WAITING		0
#define START		1
#define BLKBGN		2
#define BLKDATA		3
#define SUCCESS		4

/* Quittungen */
#define START_ACK	0x11
#define BLK_ACK		0x23
#define RPT_BLK		0x3F
#define STOP		0x21

/* Fehler */
#define CS_ERROR	-2

/* Frametypen */
#define CONTROL		1
#define DATA		2

/* Empfangsmodi */
#define BEGIN	1
#define NEXT	2

/* Anzahl Empfangsdaten */
#define ONE		1
#define ALL		2

/* Operation */
#define NONE	0
#define SEND	1
#define RECEIVE	2

/* Uebertragungsunterbrechungen */
#define CRLF	1	
#define CONT	2

/* Modus der Checksummenberechnung */
#define NOLINE		0
#define DATALINE	1

typedef struct{
	int len;
	int datatype;
	int ptr;
	int ctrl;
	int mode;	
	int cs;
	boolean markflag;
}BHEADER;

typedef struct{
	char *name;
	char *number;
	char *adress;
	char *free1;
	char *free2;
	boolean markflag;
}TELEPHONE;

typedef struct{
	int day;
	int month;
	int year;
}CDATE;

typedef struct{
	int hours;
	int minutes;
}TM;

typedef struct{
	TM start;
	TM end;
	boolean rangeflag;
}TIMERANGE;

typedef struct{
	char *descrip;
	boolean markflag;
}MEMODAT;

typedef struct{
	CDATE dat;
	TIMERANGE tm;
	char *descrip;
	TM alarm;
	boolean alarmflag;
	boolean markflag;
}SCHEDULE;

typedef struct{
	CDATE dat;
	TM alarm;
	char *descrip;
	boolean markflag;
}REMINDER;

typedef struct{
	CDATE dat;
	TM tm;
	char *descrip;
	boolean markflag;
}CALENDER;

typedef struct{
	void *ptr;
	int mode;	
}FRAME;