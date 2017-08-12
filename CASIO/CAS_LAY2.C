#include <robdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <tos.h>
#include "casio.h"

/* Layer 1 Routinen */
extern int polln(int n,char *in,int mode);
extern void sendn(int n, char *out);
extern int hexstring_to_int(int count,char *hexstring);
extern boolean string_to_hexstring(int n,char *string,char *hexstring);
extern boolean int_to_string(int i,int digits,char *string);

static boolean waiting(void);
static boolean bstart(void);
static int bheader(void);
static int getlen(int *len);
static int getdatatype(int len);
static int getptr(void);
static int getctrl(void);
static int getmode(char *indata);
static int getcs(void);
static int bdata(void);
static boolean check(BHEADER *b,char *indata);
static int getheadersum(BHEADER *b);
static int getdatasum(int n,char *data,int mode);

static boolean trying(void);
static boolean putframe(void *ptr,int mode);
static boolean outframe(BHEADER *b,char *outstring);
static boolean modechange(int mode);
static boolean endframe(boolean markflag);
static boolean termframe(void);
static boolean ack(void);
static boolean putphone(TELEPHONE *ptr);
static boolean putmemo(MEMODAT *ptr);
static boolean putremind(REMINDER *ptr);
static boolean putsched(SCHEDULE *ptr);
static boolean putcalend(CALENDER *ptr);
static boolean headerout(BHEADER *b);
static boolean dataout(BHEADER *b,char *outstring);

static void gen_datestring(CDATE *dat,char *datestring);
static void gen_timestring(TM *tm,char *timestring);

extern BHEADER blk;
extern char dataline[256];

boolean receive(int mode)
{
	int state,prevstate=UNDEF;
	boolean bres;
	int res;
	
	if(mode==BEGIN)
		state=WAITING;	
	else
		state=START;
	
	while( (state!=UNDEF) && (state!=SUCCESS)){
		switch(state){
			case WAITING:
						bres=waiting();
						if(bres==NO_ERROR)
							state=START;
						else{
							prevstate=state;
							state=UNDEF;
						}
						break;
			case START:	bres=bstart();
						if(bres==NO_ERROR){
							state=BLKBGN;
							#if DEBUG && LEVEL1
							printf("Blockstart detected\n");
							#endif
						}
						else{
							#if DEBUG && LEVEL1
							printf("Blockstart missing\n");
							#endif
							prevstate=state;
							state=UNDEF;	
						}
						break;
			case BLKBGN:res=bheader();			
						switch(res){
							case ERROR: prevstate=state;
										state=UNDEF;
										#if DEBUG && LEVEL1
										printf("Error in Blockheader\n");
										#endif
										break;
							case NO_ERROR:
										state=BLKDATA;
										#if DEBUG && LEVEL1
										printf("Blockheader detected\n");
										#endif
										break;
							case CRLF:	state=WAITING;
										#if DEBUG && LEVEL1
										printf("CRLF Interrupt occured\n");
										#endif
										break;
							case CONT:	state=START;
										#if DEBUG && LEVEL1
										printf("0x22 Interrupt occured\n");
										#endif
										break;
							
						}
						break;			
			case BLKDATA:	res=bdata();	
							switch(res){
								case CS_ERROR:
											state=START;
											#if DEBUG && LEVEL1
											printf("Blockdata repeat\n");
											#endif
											break;
								case ERROR:	prevstate=state;
											state=UNDEF;
											#if DEBUG && LEVEL1
											printf("Error in Blockdata\n");
											#endif
								case NO_ERROR:
											state=SUCCESS;
											#if DEBUG && LEVEL1
											printf("Blockdata read\n");
											#endif
											break;
								case CRLF:	state=WAITING;
											#if DEBUG && LEVEL1
											printf("CRLF Interrupt occured\n");
											#endif
											break;
								case CONT:	state=START;
											#if DEBUG && LEVEL1
											printf("0x22 Interrupt occured\n");
											#endif
											break;
							}
							break;
			case UNDEF:	switch(prevstate){
							case WAITING:
								printf("Timeout during transmission-start!\n");
								break;						
							case START:
								printf("Error in Blockstart\n");
								break;
							case BLKBGN:
								printf("Error in Blockheader\n");
								break;
							case BLKDATA:
								printf("Error in Blockdata\n");
								break;
						} /* switch(prevstate) */
		} /* switch(state) */
	}	/* while( (state!=UNDEF) && (state!=SUCCESS)) */
	
	if(state==SUCCESS)
		return(NO_ERROR);
	else
		return(ERROR);
}

static boolean waiting(void)
{
	char indata[2];	
	int again=1,timer=5;
	
	do{
		#if DEBUG && LEVEL1
		printf("Readstart...\n");
		#endif
		if(polln(2,indata,CONTROL)==NO_ERROR){	/* Zwei Bytes lesen */	
			if( (indata[0]==CAR_RET) && (indata[1]==LIN_FED)){
				Bconout(PORT,START_ACK); 
				again=0;																								
			}
		}

		--timer;
		
	}while(again && (timer>0));
	
	if(again==0){
		#if DEBUG && LEVEL1
		printf("Quittung uebermittelt\n");
		#endif	
		return(NO_ERROR);
	}
	else
		return(ERROR);
}

static boolean bstart(void)
{
	char indata[1];
	
	#if DEBUG && LEVEL1
	printf("Waiting for Blockstart ... ");
	#endif
	do{	
		if(polln(1,indata,CONTROL)==ERROR)	/* Ein Byte lesen */
			return(ERROR);
	}while((indata[0]==CAR_RET) || (indata[0]==LIN_FED));	/* CR,LF skippen */

	if(indata[0]==BLKSTART)
		return(NO_ERROR);
	else
		return(ERROR);	
}

static int bheader(void)
{
	int erg,len;

	erg=getlen(&len); 

	if(erg!=NO_ERROR)
		return(erg);
	else{
		if(len>=0)
			blk.len=len;	
		else{
			#if DEBUG && LEVEL1
			printf("Illegale Laengenangabe");
			#endif
			return(ERROR);
		}
	}

	#if DEBUG && LEVEL1
	printf("Blocklaenge: %d\n",blk.len);
	#endif
		
	erg=getdatatype(blk.len);
	if(erg>=0)
		blk.datatype=erg;
	else
		return(ERROR);
		
	#if DEBUG && LEVEL1
	printf("Datentypus: %x\n",blk.datatype);
	#endif
	
	erg=getptr();
	if(erg>=0)
		blk.ptr=erg;
	else
		return(ERROR);

	#if DEBUG && LEVEL1
	printf("Pointer: %d\n",blk.ptr);
	#endif

	erg=getctrl();
	if(erg>=0){
		blk.ctrl=erg;
			/* Bei Moduswechsel: zwei Modusbytes */
		if((blk.ctrl==MODECHG) && (blk.len!=2))	
			return(ERROR);			
	}
	else
		return(ERROR);

	#if DEBUG && LEVEL1
	printf("Steuerbyte: %x\n",blk.ctrl);
	#endif			
		
	return(NO_ERROR);
}	
	
static int getlen(int *len)
{
	char indata[3];
	int erg;
	
	erg=polln(2,indata,DATA);	/* Zwei Bytes lesen */

	if(erg!=NO_ERROR)
		return(erg);
	else{
		indata[2]=0;
			/* Bytes in Integer konvertieren */
		*len=hexstring_to_int(2,indata);	

		if(*len<0)	/* Fehlerhaftes Format */
			*len=-1;
	}		
	
	return(NO_ERROR);
}

static int getdatatype(int len)
{
	char indata[2];
	int datatype,erg;
	
	erg=polln(1,indata,DATA);	/* Ein Byte lesen */
	if(erg==ERROR)	
		return(ERROR);
	else if(erg==CRLF)
		return(CRLF);
	else{		
		indata[1]=0;		
			/* Byte in Integer konvertieren */
		datatype=hexstring_to_int(1,indata);

		if(datatype<0){
			#if DEBUG && LEVEL1
			printf("Type: %s\n",indata);
			#endif
			return(ERROR);
		}

		if(len!=0){
			switch(datatype){
				case UNKNOWN:
				case TEXT:
				case ALARM:
				case DATE:
				case TIME:		/* Gueltiger Datentyp */
							return(datatype);	
								/* Ungueltiger Datentyp */
				default: 	return(-1); 
			}	
		} /* if(len!=0) */		
		else{
			switch(datatype){			
				case MARKED:
				case UNMARKED:
								/* Gueltiger Datentyp */
							if(len==0)
								return(datatype);		
								/* Ungueltiger Datentyp */		
				default:	return(-1);
			}
		} /* else if(len!=0) */
	} /* else if(polln(1,indata,DATA)==ERROR) */
}

static int getptr(void)
{
	char indata[4];
	int ptr,erg;
	
	erg=polln(3,indata,DATA);	/* Drei Bytes lesen */
	
	if(erg!=NO_ERROR)	
		return(erg);
	else{
		indata[3]=0;
			/* Bytes in Integer konvertieren */
		ptr=hexstring_to_int(3,indata);	
		if(ptr<0){	/* Fehlerhaftes Format */
			#if DEBUG && LEVEL1
			printf("Ptr: %s\n",indata);
			#endif
			return(-1);
		}
		else	/* Korrektes Format */
			return(ptr);
	}	
}

static int getctrl(void)
{
	char indata[3];
	int ctrl,erg;
	
	erg=polln(2,indata,DATA);	/* Zwei Bytes lesen */
	
	if(erg!=NO_ERROR)
		return(erg);
	else{
		indata[2]=0;
			/* Bytes in Integer konvertieren */
		ctrl=hexstring_to_int(2,indata);	

		switch(ctrl){
			case NOCTRL:
			case SETEND:
			case MODECHG:
			case TRANSEND:	return(ctrl);
			default:
						#if DEBUG && LEVEL1
						printf("Type: %s\n",indata);
						#endif
						return(-1);
		}				
	}
}

static int getmode(char *indata)
{
	int mode;
	
	/* Bytes in Integer konvertieren */
	mode=hexstring_to_int(4,indata);	
		
	if(blk.ctrl==MODECHG){
		switch(mode){
			case PHONE:
			case MEMO:
			case REMIND:
			case SCHED:
			case CALEND:	return(mode);
			default: return(-1);
		}
	}
	else
		return(-1);
}

static int bdata(void)
{
	int memamt,cs,mode,res;
	char *indata;
	boolean erg;
	
	/* Blockl"ange + 2 Checksummen-Bytes */
	memamt=(int)(sizeof(char) * ((blk.len+1)*2));
	
	indata=(char *)malloc(memamt);

	if(indata==NULL){
		#if DEBUG && LEVEL1
		printf("Memory allocation error\n");
		#endif
		return(ERROR);	
	}
		
	res=polln(memamt,indata,DATA);	/* Blockdaten lesen */	

	if(res!=NO_ERROR)
		return(res);

	cs=hexstring_to_int(2,&indata[memamt-2]);	

	if(cs>=0)
		blk.cs=cs;
	else{
		#if DEBUG && LEVEL1
		printf("Checksum below 0: %c%c\n",indata[memamt-2],indata[memamt-1]);
		#endif
		return(ERROR);
	}

	if(blk.ctrl==MODECHG){
		mode=getmode(indata);
		
		if(mode!=-1)
			blk.mode=mode;
		else{
			#if DEBUG && LEVEL1
			printf("Modechange value illegal\n");
			#endif
			return(ERROR);
		}
	}
		
	erg=check(&blk,indata);	

	free(indata);
	
	if(erg!=ERROR){	/* Checksumme OK */		
		#if DEBUG && LEVEL1
		printf("Block quittiert\n");
		#endif	
		/* Bconout(PORT,BLK_ACK);	Block quittieren */		
		return(NO_ERROR);
	}
    else{	/* Checksummen-Fehler */
		#if DEBUG && LEVEL1
		printf("Block neu angefordert\n");
		#endif	
		/* Bconout(PORT,RPT_BLK);	Neu"ubertragung anfordern */
		return(CS_ERROR);
	}
}

static boolean check(BHEADER *b,char *indata)
{
	int headersum=0,datasum=0;
			
	headersum=getheadersum(b);
	
	datasum=getdatasum(b->len,indata,DATALINE);	
	
	if(datasum==-1)
		return(ERROR);

	if( (256-((datasum+headersum)%256)) != b->cs)
		return(ERROR);
	else
		return(NO_ERROR);

}

static int getheadersum(BHEADER *b)
{
	int headersum;

	headersum = b->len;
	headersum += (b->datatype*16) + (b->ptr/256);
	headersum += (b->ptr%256);
	headersum += b->ctrl;
	
	return(headersum);
}

static int getdatasum(int n,char *data,int mode)
{
	register int i,j;
	int newval,datasum=0;

	for(i=0,j=0;i<(n*2);i+=2,j++){
		newval=hexstring_to_int(2,&data[i]);		

		if(newval==-1){
			#if DEBUG && LEVEL1
			printf("Error calculating Checksum\n");
			#endif
			return(-1);
		}
		
		datasum+=newval;
		
		if(mode==DATALINE)
			dataline[j]=(char)(newval);
		
		#if DEBUG && LEVEL0
		printf("<%d %d>",(int)data[i],(int)data[i+1]);
		#endif
	}
	
	#if DEBUG && LEVEL0		
	printf("\n");
	#endif
	
	if(mode==DATALINE)
		dataline[j]=0;
		
	return(datasum);
}

boolean send(int mode,FRAME *f)
{
	int state,prevstate=UNDEF;
	boolean bres;
	/* int res; */
	
	if(mode==BEGIN)
		state=TRYING;	
	else
		state=START;
	
	while( (state!=UNDEF) && (state!=SUCCESS)){
		switch(state){
			case TRYING:bres=trying();
						if(bres==NO_ERROR){
							state=START;
							#if DEBUG && LEVEL1
							printf("Connection established\n");
							#endif
						}
						else{
							prevstate=state;
							state=UNDEF;
						}
						break;
			case START: bres=putframe(f->ptr,f->mode);
						if(bres==NO_ERROR)
							state=SUCCESS;
						else{
							prevstate=state;
							state=UNDEF;
						}
			case UNDEF:	switch(prevstate){
							case TRYING:	
								#if DEBUG && LEVEL1
								printf("No connection possible\n");
								#endif
								break;
						}						
						break;
		} /* switch(state) */
	}	/* while( (state!=UNDEF) && (state!=SUCCESS)) */
	
	if(state==SUCCESS)
		return(NO_ERROR);
	else
		return(ERROR);
}

static boolean trying(void)
{
	char indata[1];	
	int again=1,timer=15;
	
	do{
		#if DEBUG && LEVEL1
		printf("Trying...\n");
		#endif

		Bconout(PORT,CAR_RET);	/* "Ubertragungsstart signalisieren */
		Bconout(PORT,LIN_FED);
		
		if(polln(1,indata,CONTROL)==NO_ERROR){	/* Ein Bytes lesen */	
			if(indata[0]==START_ACK)	/* Quittung erhalten */
				again=0;									
			else{
				#if DEBUG && LEVEL1
				printf("Wrong acknowledgement: %x\n",(int)indata[0]);
				#endif
			}
																			
		}
		else{
			#if DEBUG && LEVEL1
			printf("No acknowledgement\n");
			#endif
		}

		--timer;
		
	}while(again && (timer>0));
	
	if(again==0){
		#if DEBUG && LEVEL1
		printf("Quittung erhalten\n");
		#endif	
		return(NO_ERROR);
	}
	else
		return(ERROR);
}

static boolean putframe(void *ptr,int mode)
{
	boolean markflag;
	
	TELEPHONE *t;
	MEMODAT *m;
	REMINDER *r;
	SCHEDULE *s;
	CALENDER *c;

	if(modechange(mode)==ERROR)
		return(ERROR);	
	
	switch(mode){
		case PHONE:	t=(TELEPHONE *)ptr;
					if(putphone(t)==ERROR)
						return(ERROR);
					else
						markflag=t->markflag;					
					break;
		case MEMO:	m=(MEMODAT *)ptr;
					if(putmemo(m)==ERROR)
						return(ERROR);
					else
						markflag=m->markflag;
					break;
		case REMIND:r=(REMINDER *)ptr;
					if(putremind(r)==ERROR)
						return(ERROR);
					else
						markflag=r->markflag;
					break;
		case SCHED: s=(SCHEDULE *)ptr;
					if(putsched(s)==ERROR)
						return(ERROR);
					else
						markflag=s->markflag;
					break;
		case CALEND:c=(CALENDER *)ptr;
					if(putcalend(c)==ERROR)
						return(ERROR);
					else
						markflag=c->markflag;
					break;
	}
	
	return(endframe(markflag));
}

static boolean modechange(int mode)
{
	BHEADER bh;	
	char outstring[5];
	
	bh.len=2;
	bh.datatype=0;
	bh.ptr=0;
	bh.ctrl=2;		
	bh.mode=mode;	

	switch(mode){
		case PHONE:	strcpy(outstring,"9000");
					break;
		case MEMO:	strcpy(outstring,"A000");
					break;
		case REMIND:strcpy(outstring,"9100");
					break;
		case SCHED:	strcpy(outstring,"B000");
					break;
		case CALEND:strcpy(outstring,"8000");
					break;
	}

	if(headerout(&bh)==ERROR)
		return(ERROR);
	if(dataout(&bh,outstring)==ERROR)
		return(ERROR);	
				
	#if DEBUG && LEVEL1
	printf("Modechange successfully written\n");
	#endif			
	
	return(NO_ERROR);
}

static boolean endframe(boolean markflag)
{
	BHEADER bh;	
	char dummy[3];	/* Platz f"ur Checksumme! */
	
	dummy[0]=0;
	
	bh.len=0;
	if(markflag)
		bh.datatype=MARKED;
	else
		bh.datatype=UNMARKED;
	bh.ptr=0;
	bh.ctrl=SETEND;		
	bh.mode=0;	

	if(headerout(&bh)==ERROR)
		return(ERROR);

	if(dataout(&bh,dummy)==ERROR)
		return(ERROR);
				
	return(ack());
}

static boolean termframe(void)
{
	BHEADER bh;	
	char dummy[3];	/* Platz f"ur Checksumme! */
	
	dummy[0]=0;
	
	bh.len=0;
	bh.datatype=0;
	bh.ptr=0;
	bh.ctrl=TRANSEND;		
	bh.mode=0;	

	if(headerout(&bh)==ERROR)
		return(ERROR);

	if(dataout(&bh,dummy)==ERROR)
		return(ERROR);
				
	return(NO_ERROR);
}

static boolean ack(void)
{
	char indata[1];
	int tries=5;
	boolean succ=FALSE;

	do{

	if(polln(1,indata,CONTROL)==NO_ERROR){	/* Ein Bytes lesen */				
	
		switch((int)indata[0]){
			case BLK_ACK:
				#if DEBUG && LEVEL1
				printf("Block acknowledged\n");
				#endif
				tries=0;
				succ=TRUE;
				break;
			case RPT_BLK:
				#if DEBUG && LEVEL1
				printf("Block repeat wished\n");
				#endif
				tries=0;
				succ=TRUE;
				break;				
			case STOP:
				#if DEBUG && LEVEL1
				printf("Stop wished\n");
				#endif
				tries=0;
				succ=TRUE;
				break;			
			default:
				#if DEBUG && LEVEL1
				printf("Unknown acknowledgement: %x\n",(int)indata[0]);
				#endif
				break;
		}	
	}	
	else{
		--tries;
		#if DEBUG && LEVEL1
		printf("No acknowledgement\n");
		#endif				
	}
		
	}while(tries!=0);
	
	return(succ==TRUE);
}

static boolean putphone(TELEPHONE *ptr)
{
	/*
	typedef struct{
	char *name;
	char *number;
	char *adress;
	char *free1;
	char *free2;
	boolean markflag;
	}TELEPHONE;
	*/

	if(ptr);

	return(NO_ERROR);
}

static boolean putmemo(MEMODAT *ptr)
{
	if(ptr);

	return(NO_ERROR);	
}

static boolean putremind(REMINDER *ptr)
{	
	char datestring[11],timestring[6];	
	char outstring[512];
	BHEADER b;
		
	#if DEBUG && LEVEL1
	printf("Remind-Data out\n");
	#endif	
		
	gen_datestring(&ptr->dat,datestring);			

	b.len=(int)strlen(datestring);
	b.datatype=DATE;
	b.ptr=0;
	b.ctrl=0;
	b.mode=0;			
	if(string_to_hexstring(b.len,datestring,outstring)==ERROR)
		return(ERROR);
	if(outframe(&b,outstring)==ERROR)
		return(ERROR);		

	#if DEBUG && LEVEL1
	printf("Date successfully written\n");
	#endif


	gen_timestring(&ptr->alarm,timestring);	
	
	b.len=(int)strlen(timestring);
	b.datatype=ALARM;
	b.ptr=0;
	b.ctrl=0;
	b.mode=0;		
	if(string_to_hexstring(b.len,timestring,outstring)==ERROR)
		return(ERROR);
	if(outframe(&b,outstring)==ERROR)
		return(ERROR);

	#if DEBUG && LEVEL1
	printf("Time successfully written\n");
	#endif
	
	b.len=(int)strlen(ptr->descrip);
	b.datatype=TEXT;
	b.ptr=0;
	b.ctrl=0;
	b.mode=0;	
	if(string_to_hexstring(b.len,ptr->descrip,outstring)==ERROR)
		return(ERROR);	
	if(outframe(&b,outstring)==ERROR)
		return(ERROR);
	
	#if DEBUG && LEVEL1
	printf("Reminder successfully written\n");
	#endif
	
	return(NO_ERROR);
}

static void gen_datestring(CDATE *dat,char *datestring)
{
	int len;
	char helpstring[5];
		
	datestring[0]=0;

	if(dat->year!=0)
		itoa(dat->year,helpstring,10);
	else
		strcpy(helpstring,"----");
		
	strcat(datestring,helpstring);
	strcat(datestring,"-");

	itoa(dat->month,helpstring,10);	
	len=(int)strlen(helpstring);
	if(len<2){	/* F"uhrende Null erzeugen */
		helpstring[2]=0;
		helpstring[1]=helpstring[0];
		helpstring[0]='0';
	}
	strcat(datestring,helpstring);
	strcat(datestring,"-");

	itoa(dat->day,helpstring,10);
	len=(int)strlen(helpstring);	
	if(len<2){	/* F"uhrende Null erzeugen */
		helpstring[2]=0;
		helpstring[1]=helpstring[0];
		helpstring[0]='0';
	}
	strcat(datestring,helpstring);
}

static void gen_timestring(TM *tm,char *timestring)
{
	int len;
	char helpstring[5];
	
	timestring[0]=0;
		
	itoa(tm->hours,helpstring,10);
	len=(int)strlen(helpstring);	
	if(len<2){	/* F"uhrende Null erzeugen */
		helpstring[2]=0;
		helpstring[1]=helpstring[0];
		helpstring[0]='0';
	}
	strcat(timestring,helpstring);
	strcat(timestring,":");

	itoa(tm->minutes,helpstring,10);
	len=(int)strlen(helpstring);	
	if(len<2){	/* F"uhrende Null erzeugen */
		helpstring[2]=0;
		helpstring[1]=helpstring[0];
		helpstring[0]='0';
	}
	strcat(timestring,helpstring);	
}


static boolean putsched(SCHEDULE *ptr)
{
	/*
	typedef struct{
	CDATE dat;
	TIMERANGE tm;
	char *descrip;
	TM alarm;
	boolean alarmflag;
	}SCHEDULE;
	*/
	
	if(ptr);
	
	return(NO_ERROR);
}

static boolean putcalend(CALENDER *ptr)
{	
	/*
	typedef struct{
	CDATE dat;
	TM tm;
	char *descrip;
	}CALENDER;
	*/
	
	if(ptr);
	
	return(NO_ERROR);
}


static boolean outframe(BHEADER *b,char *outstring)
{
	if(headerout(b)==ERROR)
		return(ERROR);
	if(dataout(b,outstring)==ERROR)
		return(ERROR);
	
	return(NO_ERROR);
}

static boolean headerout(BHEADER *b)
{
	char outdata[10],string[4];
	int digits[4]={2,1,3,2};
	int *ptr;
	register int i;

	outdata[0]=':';
	outdata[1]=0;
	
	ptr=(int *)b;	/* Pointer auf ersten Integerwert */
	
	for(i=0;i<4;i++){
		if(int_to_string(*(ptr+i),digits[i],string)==ERROR)
			return(ERROR);
		else
			strcat(outdata,string);
	}			
		
	sendn(9,outdata);	
	
	return(NO_ERROR);
}

static boolean dataout(BHEADER *b,char *outdata)
{
	char string[3];
	int headersum,datasum;

	headersum=getheadersum(b);
	
	if(b->len!=0)
		datasum=getdatasum(b->len,outdata,NOLINE);
	else
		datasum=0;

	b->cs=(256-((datasum+headersum)%256));
	
	int_to_string(b->cs,2,string);
	strcat(outdata,string);
	
	sendn(((b->len+1)*2),outdata);	
	
	return(NO_ERROR);
	
 	/* return(ack()); */
}