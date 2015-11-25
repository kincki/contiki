/**
 *  Energy Monitoring & Management Agent for IPV6 RestFull HTTP 
 *  Copyright (C) 2010  DUHART Clément <duhart.clement@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * All rights reserved.
 *
 * Documentation : http://www.ece.fr/~duhart//emma/doc/html/
 *
 *
 * \addtogroup EMMA
 *
 * @{
 */

/**
 * \file
 *         Header file of EMMA interpreter for EMMALanguage
 * \author
 *         Clement DUHART <duhart.clement@gmail.com>
 */
#include "Emma-Rest6.h"

extern struct System slash;
RESSOURCE_LIST_LOAD(ressources_list);

const char op_AND[]    _PROGMEM = {"&&\0"};
const char op_OR[]     _PROGMEM = {"||\0"};
const char op_INF[]    _PROGMEM = {"<\0"};
const char op_SUP[]    _PROGMEM = {">\0"};
const char op_EGU[]    _PROGMEM = {"==\0"};
const char op_PLUS[]   _PROGMEM = {"+\0"};
const char op_MINUS[]  _PROGMEM = {"-\0"};
const char op_DIV[]    _PROGMEM = {"/\0"};
const char op_MODULO[] _PROGMEM = {"%\0"};
const char op_TIME[]   _PROGMEM = {"*\0"};
const char op_COMP[]   _PROGMEM = {"!\0"};
const char op_THIS[]   _PROGMEM = {"this.\0"};
const char op_THIS_TIME[]   _PROGMEM = {"time\0"};

enum {
AND,
OR,
INF,
SUP,
EGU,
PLUS,
MINUS,
DIV,
MODULO,
TIME,
COMP,
THIS,
THIS_TIME
};

int getOperande(char * str, char opT, char * opd1, char * opd2, int size){
	int i=0,j;
	char ok;
	char * temp;
	char op[7];

	switch(opT){
		case AND    : strcpy_P(op,op_AND);    break;
		case OR     : strcpy_P(op,op_OR);     break;
		case INF    : strcpy_P(op,op_INF);    break;
		case SUP    : strcpy_P(op,op_SUP);    break;
		case EGU    : strcpy_P(op,op_EGU);    break;
		case PLUS   : strcpy_P(op,op_PLUS);   break;
		case MINUS  : strcpy_P(op,op_MINUS);  break;
		case DIV    : strcpy_P(op,op_DIV);    break;
		case MODULO : strcpy_P(op,op_MODULO); break;
		case TIME   : strcpy_P(op,op_TIME);   break;
		case COMP   : strcpy_P(op,op_COMP);   break;
		case THIS   : strcpy_P(op,op_THIS);   break;
		case THIS_TIME   : strcpy_P(op,op_THIS_TIME);   break;
		default : return 0;
	}

	_PRINTF("Emmal_evaluate : Read logic bloc : process starting...\n");

	temp = strstr(str,op);
	if(temp == NULL)	return 0;
	temp += strlen(op);
	//printf("Temp%s\n",temp);

	_PRINTF("Emmal_evaluate : Read logic bloc : Operande1\n");

	i=0;
	ok=0;
	while(i < strlen(str) && i < size){

		ok = 1;
		for(j=0;j<strlen(op);j++)
			if(str[i+j] != op[j])
				ok = 0;

		if(ok == 1) break;

		opd1[i] = str[i];
		i++;
		}
	opd1[i] = '\0';

	_PRINTF("Emmal_evaluate : Read logic bloc : Operande2\n");

	i=0;
	ok=0;
	while(i < strlen(temp) && i < size){


		opd2[i] = temp[i];
		i++;
		}
	opd2[i] = '\0';

//	printf("Op1:%s\nOp2:%s\nOp:%s\n",opd1,opd2,op);
	_PRINTF("Emmal_evaluate : Read logic bloc : [SUCCESS]\n");
	return 2;
	}

int Emmal_evaluate (char* str){
	static struct Ressource * PtRessource;
	char opd1[strlen(str)];
	char opd2[strlen(str)];
	static int res1;
	static int res2;
	static char op = -1;
	_PRINTF("Emmal_evaluate : Start processing ...\n");

	if 	(getOperande(str,COMP,opd1, opd2,strlen(str)) == 2)		op = COMP;
	else if	(getOperande(str,AND,opd1, opd2,strlen(str)) == 2)		op = AND;
	else if	(getOperande(str,OR,opd1, opd2,strlen(str)) == 2)		op = OR;
	else if (getOperande(str,INF,opd1, opd2,strlen(str)) == 2)		op = INF;
	else if (getOperande(str,SUP,opd1, opd2,strlen(str)) == 2)		op = SUP;
	else if (getOperande(str,PLUS,opd1, opd2,strlen(str)) == 2)		op = PLUS;
	else if (getOperande(str,MINUS,opd1, opd2,strlen(str)) == 2)		op = MINUS;
	else if (getOperande(str,DIV,opd1, opd2,strlen(str)) == 2)		op = DIV;
	else if (getOperande(str,TIME,opd1, opd2,strlen(str)) == 2)		op = TIME;
	else if (getOperande(str,MODULO,opd1, opd2,strlen(str)) == 2)		op = MODULO;
	else if (getOperande(str,THIS,opd1, opd2,strlen(str)) == 2){
		_PRINTF("Emmal_evaluate : Found variable\n");

		if(strcmp(opd2,THIS_TIME) == 0)
			return slash.time;

		else for(PtRessource = list_head(ressources_list);PtRessource != NULL; PtRessource = PtRessource->next)
			if(strcmp(PtRessource->name,opd2) == 0)
				return PtRessource->value; 

	return 0;
	}
	else return atoi(str);

	res1 = Emmal_evaluate(opd1);
	res2 = Emmal_evaluate(opd2);

	if (op == AND)			return (res1 && res2);
	else if (op == OR)		return (res1 || res2);
	else if (op == INF)		return (res1 < res2);
	else if (op == SUP)		return (res1 > res2);
	else if (op == PLUS)		return (res1 + res2);
	else if (op == MINUS)		return (res1 - res2);
	else if (op == DIV)		return (res1 / res2);
	else if (op == TIME)		return (res1 * res2);
	else if (op == MODULO)		return (res1 % res2);
	else if (op == COMP)		return !res2;
	_PRINTF("Emmal_evaluate : [SUCCESS]\n");

	return 0;
	}

/*
int main (int * argv, char * argc[]){

char input[50];
char * temp;
strcpy(input,"-10<-20||5>2&&10>6&&1<2 || this.toto < 10\n");
printf("interpreteur : %s\n",input);
printf("End : %d\n",Emmal_evaluate(input));
return 0;
}
*/
/*
int parse(char * input, char * temp, int val, int res){
char buf[3];
int i = 0;

if(temp != NULL){
	res = Emmal_evaluate((temp+1),val);
	int a=-1,b=-1;
	// End of the expression
	if(res == -1){
		printf("operateur : %s\n", temp);
		}
	else 	{
		printf("operande : %d\n",res);
			// input contient l'operande
			b = res;
			temp = input;
			while (temp[0] != ' ') buf[i++] = *temp++;
			buf[i] = '\0';
			printf("operande2 : %s\n",buf);
			return (res);

		// temp contient l'operateur
		i = 0;
		while (temp[0] != ' ') buf[i++] = *temp++;
		buf[i] = '\0';
		printf("operateur : %s\n",buf);

		if(strcmp (buf,"&") == 0) {

			// input contient l'operande
			b = res;
			temp = input;
			while (temp[0] != ' ') buf[i++] = *temp++;
			buf[i] = '\0';
			printf("operande2 : %s\n",buf);
			return (res);
		}
		else if(strcmp(buf,"<") == 0){

		}
		else if(strcmp(buf,">") == 0){

		}
		else {

		}
	}
	
	return res;
	}

	// Aucun operateur donc operande ?
 return atoi(input);

}


int Emmal_evaluate(char * input, int val){
loop++;
int res=-1;
char * temp;

printf("%d-Emmal_evaluate(%d) : %s \n",loop,val, input);

temp = strstr(input,"&");
printf("& : %s\n",temp);
if(temp != NULL){
	printf("<=");
 return parse(input,temp, val,res);
	  }
temp = strstr(input,"<");
printf("< : %s\n",temp);
if(temp != NULL){
	printf("<=");
 return parse(input,temp, val,res);
	  }

temp = strstr(input,">");
printf("> : %s\n",temp);
if(temp != NULL){
	printf("<=");
 return parse(input,temp, val,res);
	  }

 return atoi(input);
}






/*
enum {
AND,
INF,
SUP
};

int Emmal_evaluate(char * input){
int res = -1;
char op = -1;
char buf[5];
int i;
char * temp;
loop++;

if(strstr(input,">") != NULL ||
		strstr(input,"<") != NULL ||
			strstr(input,"&&") != NULL){

	temp = strstr(input,"&");
	if(temp == NULL) 	temp = strstr(input,"<");
	if(temp == NULL) 	temp = strstr(input,">");

	printf("Emmal_evaluate(%d) before call : %s\n",loop,temp);
	// Recherche de l'operande 2
	res = Emmal_evaluate((temp+1));
	printf("operande2 : %d\n",res);

	// Recherche de l'operateur
	printf("temp : %s \n",temp);
	i=0;
	while (temp[0] != ' ') buf[i++] = *temp++;
	buf[i] = '\0';
	printf("buf : %s \n",buf);

	if(strcmp(buf,"&") == 0) op = AND;
	if(strcmp(buf,"<") == 0) op = INF;
	if(strcmp(buf,">") == 0) op = SUP;
	printf("operateur : %d\n",op);
		
	// Recherche de l'operande 1
	temp = input+1;
	printf("temp2 : %s\n",temp);
	i=0;
	while (temp[0] != ' ') buf[i++] = *temp++;
	buf[i] = '\0';
	printf("buf2 : %s \n",buf);
	printf("operande1 : %d\n",atoi(buf));

	switch(op){
		case AND : return (res && atoi(buf)); break;
		case INF : return (res < atoi(buf)); break;
		case SUP : return (res > atoi(buf)); break;
	}

	}
else {
	res = atoi(input);
	printf("Emmal_evaluate(%d) end : %d\n",loop,res);
	}

return res;

}
*/



/** @} */
