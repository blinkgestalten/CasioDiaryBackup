#include <robdefs.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "casio.h"


/* Layer 1 Routinen */
extern void clearbuffer(void);
extern void sendack(void);

/* Layer 2 Routinen */
extern boolean receive(int mode);
boolean send(int mode,FRAME *f);

static void recv(int amount);
static void snd(int amount);
static int getframe(void);

BHEADER blk;
char dataline[256];

#if LOGWRITE || SIM
FILE *log;
#endif

#ifdef OUTFILE
FILE *outfile;
#endif

void main(void)
{
	char input;	
	int op=NONE;
	
	#if LOGWRITE
	log=fopen("E:\\TC\\PROG\\CASIO\\CASIO.LOG","wb");
	#endif
	
	#if SIM
	log=fopen("E:\\TC\\PROG\\CASIO\\CASIO.LOG","rb");
	#endif

	#ifdef OUTFILE
	outfile=fopen("E:\\TC\\PROG\\CASIO\\CASIO.OUT","wb");
	#endif

	#if LOGWRITE || SIM
	if(log==NULL){
		printf("Error opening Logfile");
		return;
	}
	#endif
	
	clearbuffer();
	
	#if DEBUG && LEVEL1
	printf("Buffer cleared\n");
	#endif	

	printf("<s> Senden\n");
	printf("<e> Empfangen\n");

	input=(char)getchar();
	
	if(input=='s')
		op=SEND;
	else if(input=='e')
		op=RECEIVE;

	if(op==SEND){
		printf("\n<1> Einen Datensatz senden\n");
		printf("<n> n Datensaetze senden\n");

		do{
			input=(char)getchar();
		}while(input=='\n');
	
		if(input=='1')
			snd(ONE);
		else if(input=='n')
			snd(ALL);	
	}	
	else if(op==RECEIVE){
		printf("\n<1> Einen Datensatz empfangen\n");
		printf("<n> n Datensaetze empfangen\n");
	
		do{
			input=(char)getchar();
		}while(input=='\n');
	
		if(input=='1')
			recv(ONE);
		else if(input=='n')
			recv(ALL);
	}	
	
	#if LOGWRITE || SIM
	fclose(log);
	#endif
	
	#ifdef OUTFILE
	fclose(outfile);
	#endif
}

static void recv(int amount)
{
	int ftype;	
	boolean endflag=FALSE;	

	do{
		ftype=getframe();
		
		switch(ftype){
			case ERROR:		
							#if DEBUG && LEVEL3
							printf("Error occured\n");
							#endif							
							endflag=TRUE;
							break;		
			case CONTROL:	
							#if DEBUG && LEVEL3
							printf("Controlframe received\n");
							#endif
							if(blk.ctrl==TRANSEND ||
							((blk.ctrl==SETEND) && (amount==ONE)) )
								endflag=TRUE;
							break;
			case DATA:		
							#if DEBUG && LEVEL3
							printf("Dataframe received\n");
							#endif
							break;			
		}
	}while(endflag==FALSE);
	
}

static int getframe(void)
{
	static boolean firstcall=TRUE;
	int ftype;
	boolean erg;
	
	FILE *fptr;
	
	fptr=fopen("E:\\TC\\PROG\\CASIO\\CASIO.DAT","a");

	if(fptr==NULL){
		#if DEBUG && LEVEL2
		printf("Error opening Datafile\n");	
		#endif
		return(ERROR);
	}
	
	if(firstcall){
		erg=receive(BEGIN);
		firstcall=FALSE;
	}
	else
		erg=receive(NEXT);

	if(erg==ERROR)
		return(ERROR);

	if( (blk.len==0) || ((blk.len==2)&&(blk.ctrl==2)) ){
		#if DEBUG && LEVEL2
		printf("KONTROLLFRAME\n");
		#endif
		
		ftype=CONTROL;
				
		switch(blk.datatype){
			case MARKED:
						#if DEBUG && LEVEL2
						printf("Marked Block\n");
						#endif
						break;
			case UNMARKED:
						#if DEBUG && LEVEL2
						printf("Unmarked Block\n");
						#endif
						break;
			default:	
						#if DEBUG && LEVEL2
						printf("Datatype unknown\n");
						#endif
						break;
		}		
	} /* if(blk.len==0) */
	else{
		#if DEBUG && LEVEL2
		printf("DATENFRAME\n");
		#endif	
		
		ftype=DATA;
		
		switch(blk.datatype){
			case UNKNOWN:
						#if DEBUG && LEVEL2
						printf("Unknown data\n");
						#endif
						break;
			case TEXT:	
						#if DEBUG && LEVEL2
						printf("Textdata\n");
						#endif
						break;
			case ALARM:	
						#if DEBUG && LEVEL2
						printf("Alarmtime\n");
						#endif
						break;
			case DATE:	
						#if DEBUG && LEVEL2
						printf("Date\n");
						#endif
						break;
			case TIME:	
						#if DEBUG && LEVEL2
						printf("Time\n");
						#endif
						break;
		}						
		
		fprintf(fptr,"%s\n",dataline);
		printf("*** %s\n",dataline);
		
	} /* else if(blk.len==0) */
	
	switch(blk.ctrl){
		case NOCTRL:
					#if DEBUG && LEVEL2
					printf("Kein Kontrollbyte\n");
					#endif
					break;		
		case SETEND:
					#if DEBUG && LEVEL2
					printf("End of Set\n");
					#endif
					sendack();
					break;
		case TRANSEND:
					#if DEBUG && LEVEL2
					printf("End of Transmission\n");
					#endif					
					break;						
		case MODECHG:
					#if DEBUG && LEVEL2
					printf("Modechange\n");
					#endif
					break;		
	}
	
	fclose(fptr);
	
	return(ftype);

}

void snd(int amount)
{
	FRAME f;
	REMINDER r;
	
	r.descrip=malloc(sizeof(char)*11);
	
	r.dat.day=19;
	r.dat.month=1;
	r.dat.year=0;
	r.alarm.hours=12;
	r.alarm.minutes=11;	
	strcpy(r.descrip,"Hallo Test");
	r.markflag=TRUE;

	f.ptr=(void *)&r;
	f.mode=REMIND;

	if(amount==ONE)
		send(BEGIN,&f);	
	else if(amount==ALL)
		return;	
}






