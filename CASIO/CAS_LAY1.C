#include <robdefs.h>
#include <stdio.h>
#include <tos.h>
#include <ctype.h>
#include <string.h>
#include "casio.h"

#if LOGWRITE || SIM
extern FILE *log;
#endif

#ifdef OUTFILE
extern FILE *outfile;
#endif

int polln(int n,char *in,int mode);
boolean int_to_string(int i,int digits,char *string);
static boolean poll(void);
static int onebyte_to_int(char byte);
static char int_to_byte(int i);

/********************************/
/* Loescht den Empfangsbuffer	*/
/* -> void						*/
/* <- void						*/
/********************************/
void clearbuffer(void)
{
	#ifndef SIM
	char in[1];
	
	while(poll()!=ERROR){
		polln(1,in,CONTROL);
	}		
	#endif
}

/************************************************/
/* Liest n aufeinanderfolgende Zeichen von AUX  */
/* -> int n: Anzahl zu lesender Zeichen			*/
/* 	  char *in: Array zur Aufnahme der Zeichen	*/
/* <- boolean: ERROR, NO_ERROR abh. v. Timeout	*/
/************************************************/
int polln(int n,char *in,int mode)
{	
	char prev=0;

	#if DEBUG && LEVEL0
	cr=0;	
	#endif
	
	#ifdef SIM
	int count;
	#endif
	
	#ifndef SIM
	register int i;
	boolean erg;

	for(i=0;i<n;i++){
		erg=poll();
		if(erg!=ERROR){
			*(in+i)=(char)Bconin(PORT);			

			if(mode==DATA){
				if(isxdigit(*(in+i))==0){
					switch(*(in+i)){
						case 0x0A:	if(prev==0x0D);
									return(CRLF);
						case 0x0D:	prev=0x0D;
									break;
						case 0x22:	return(CONT);
						default:	prev=*(in+i);
									break;
					}
				}
			}

			#if DEBUG && LOGWRITE
			fprintf(log,"%c",*(in+i));
			#endif

			#if DEBUG && LEVEL0
			printf(">%d<",(int)*(in+i));
			cr++;
			if(cr==15){
				printf("\n");
				cr=0;
			}
			#endif
		}
		else
			return(ERROR);
		
	}	
	
	#if DEBUG && LEVEL0
	printf("\n");
	#endif

	return(NO_ERROR);
	#endif
	
	
	#ifdef SIM
	count=(int)fread(in,sizeof(char),n,log);
	
	if(count==n)
		return(NO_ERROR);
	else
		return(NO_ERROR);	
	#endif
}

/************************************************/
/* Prueft auf anliegendes Zeichen				*/
/* -> void										*/
/* <- boolean: ERROR, NO_ERROR abh. v. Timeout	*/
/************************************************/
static boolean poll(void)
{
	#ifndef SIM
	int timer=10000;
		
	while((Bconstat(PORT)==0) && (timer>0))
		--timer;
	if(timer<=0)
		return(ERROR);
	else
		return(NO_ERROR);
	#endif
	
	#ifdef SIM
	return(NO_ERROR);
	#endif
}

/************************************************/
/* Quittiert einen empfangenen Datenblock		*/
/************************************************/
void sendack(void)
{
	#if DEBUG && LEVEL1
	printf("Block quittiert\n");
	#endif
	Bconout(PORT,BLK_ACK);
}

/************************************************/
/* Schreibt n Zeichen eines Strings auf AUX		*/
/* -> int n: Anzahl zu schreibender Zeichen		*/
/* 	  char *out: Ausgabezeichenkette			*/
/* <- void:										*/
/************************************************/
void sendn(int n, char *out)
{
	register int i;

	
	for(i=0;i<n;i++){
		Bconout(PORT,*(out+i));

		#ifdef OUTFILE
		fprintf(outfile,"%c",*(out+i));
		#endif
	}
}

/****************************************************/
/* Konvertiert ASCII-Hexstring in Integer-Hexwert	*/
/* -> int count: L"ange des Hexstrings				*/
/* 	  char *hexstring: Pointer auf ASCII-Hexstring	*/
/* <- int: Integer-Hexwert, -1 im Fehlerfall		*/
/****************************************************/
int hexstring_to_int(int count,char *hexstring)
{
	int enderg,erg,faktor;
	register int i;
	
	enderg=0;
	faktor=1;
	
	for(i=count-1;i>=0;i--){	/* Bytes durchlaufen */
		erg=onebyte_to_int(*(hexstring+i));	/* Hexbyte in int Wandeln */
		if(erg>=0){
			erg*=faktor;
			enderg+=erg;
		}
		else
			return(-1);		
			
		faktor*=16;
	}
	
	return(enderg);		
}

/****************************************************/
/* Konvertiert ASCII-Hexzeichen in Integer-Hexwert	*/
/* -> char byte: ASCII-Hexzeichen					*/
/* <- int: Integer-Hexwert, -1 im Fehlerfall		*/ 
/****************************************************/
static int onebyte_to_int(char byte)
{
	if(isxdigit((int)byte)==0)
		return(-1);
	else{
		if(byte<='9')
			return((int)(byte-'0'));
		else if(byte>='A');
			return((int)(byte-'A'+10));
	}		
}

/****************************************************/
/* Konvertiert String in ASCII-Hexstring			*/
/* -> int n: Stringl"ange							*/
/*	  char *string: Pointer auf String				*/
/* 	  char *hexstring: Pointer auf ASCII-Hexstring	*/
/* <- boolean: ERROR, NO_ERROR abh. v. Fehler		*/
/****************************************************/
boolean string_to_hexstring(int n,char *string,char *hexstring)
{
	register int i;
	char str[3];

	hexstring[0]=0;
	
	for(i=0;i<n;i++){
		if(int_to_string((int)string[i],2,str)!=ERROR)
			strcat(hexstring,str);		
		else
			return(ERROR);
	}
	
	return(NO_ERROR);
}

/****************************************************/
/* Konvertiert Integerwert in ASCII-Hexstring		*/
/* -> int i: Integerwert							*/
/* 	  char *string: Pointer auf ASCII-Hexstring		*/
/* <- boolean: ERROR, NO_ERROR abh. v. Fehler		*/
/****************************************************/
boolean int_to_string(int i,int digits,char *string)
{
	unsigned int d,erg;
	char cerg;
	register int j;	

	if((unsigned int)i>=65535L)
		return(ERROR);

	d=1<<((digits-1)*4);
	
	for(j=0;j<digits;j++,(d>>=4)){
		if(i>=d){	/* Falls Stelle besetzt */
			erg=i/d;	/* Wert der Stelle ermitteln */
			cerg=int_to_byte(erg);	/* In Hexzeichen wandeln */
			if((int)cerg!=-1){	/* Wandlung korrekt */
				string[j]=cerg;	/* String f"ullen */
				i-=(erg*d);	
			}		
			else{
				#if DEBUG && LEVEL1
				printf("Error converting int to byte\n");
				#endif
				return(ERROR);				
			}
		}
		else{
			string[j]='0';
		}
	}
	
	string[j]=0;	/* String terminieren */
	
	return(NO_ERROR);
}

/****************************************************/
/* Konvertiert Integerzahl in Hexzeichen			*/
/* -> char i: Integerwert							*/
/* <- char: Hexzeichen, -1 im Fehlerfall			*/ 
/****************************************************/
static char int_to_byte(int i)
{
	if((i<0) || (i>15))
		return(-1);
	else{
		if(i<=9)
			return((char)(i+'0'));
		else
			return((char)(i-10+'A'));
	}	
}

