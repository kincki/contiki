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
 * \addtogroup cfs
 *
 * @{
 */

/**
 * \file
 *         EMMA-Rest6 CFS Engine
 * \author
 *         Clement DUHART <duhart.clement@gmail.com>
 *
 *          The file contains an API for saving data inside static slot.\n
 *          This API is used by the report and log mechanism to make a persistant abstract layer.
 */

#include "cfs/cfs.h"
#include "dev/eeprom.h"
#include <string.h>
#include "Emma-Rest6.h"



/*

DELETE 	/data/{*|RessourceName}/report/{*|IdReport}/
GET 		/data/{*|RessourceName}/
GET 		/data/{*|RessourceName}/report/
POST 		/data/{*|RessourceName}/report/
PUT 		/data/{*|RessourceName}/
PUT 		/data/{*|RessourceName}/report/{IdReport}



buf.id = 12;
REPORT_NEW (&buf, temp);
buf.id = 13;
REPORT_NEW (&buf, temp);
buf.id = 14;
REPORT_NEW (&buf, temp);
buf.id = 15;
REPORT_NEW (&buf, temp);
buf.id = 16;
REPORT_NEW (&buf, temp);

REPORT_READ(&buf, CFS_BEGIN);
while(REPORT_READ(&buf, CFS_NEXT) == 0);
*/

/*---------------------------------------------------------------------------*/
/**
 * \brief      Driver function for updating a data slot in EEPROM memory.
 * \param data DataStructure to update.
 * \param Buff  Buffer with minimal size of sizeof(struct Loggable)
 * \param root File pointer on the EEPROM memory.
 * \param size Size of the dataStructure to update.
 * \param cfs_offset Address memory of the first slot
 * \param cfs_offset_end Address memory of the last slot

 * \retval 0   Success in updating.
 * \retval 1   Fail in updating.
 *
 */
char file_update(void * data, char * Buff, struct File * root, int size, int cfs_offset, int cfs_offset_end){

	_PRINTF("UPDATE : ");
	eeprom_read(CFS_SYSTEM_OFFSET, (unsigned char*) &root, sizeof(struct File));

	root->pt = cfs_offset;

		_PRINTF("READ_SPACE ");
	while(root->pt <  cfs_offset_end) {

		eeprom_read(root->pt, (unsigned char*) Buff, sizeof(struct Loggable));
		root->pt += size;
		_PRINTF("*");

		if(((struct Loggable *)Buff)->id == ((struct Loggable*)data)->id && ((struct Loggable*)data)->id != -1){ // Update report
			_PRINTF("EMPTY => WRITING... ");
			((struct Loggable*)data)->state = USED;
			root->pt -= size;
			eeprom_write(root->pt, (unsigned char *) data, size);
			break;
		}
		else {
			_PRINTF("[USED]");
		}
	}

	if(root->pt >= cfs_offset_end){
		_PRINTF(" [FAILED]\n");
		return -1;
	}

	_PRINTF(" [SUCCESS]\n");

return 0;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief      Driver function for saving a new data slot in EEPROM memory.
 * \param data DataStructure to save.
 * \param Buff  Buffer with minimal size of sizeof(struct Loggable)
 * \param root File pointer on the EEPROM memory.
 * \param size Size of the dataStructure to save.
 * \param cfs_offset Address memory of the first slot
 * \param cfs_offset_end Address memory of the last slot

 * \retval 0   Success in saving.
 * \retval 1   Fail in saving.
 *
 */
char file_new (void * data, char * Buff, struct File * root, int size, int cfs_offset, int cfs_offset_end){

	static unsigned int i = 0;
	_PRINTF("NEW : ");

	eeprom_read(CFS_SYSTEM_OFFSET, (unsigned char*) &root, sizeof(struct File));

	root->pt = cfs_offset;

	_PRINTF("READ_SPACE ");
	while(root->pt <  cfs_offset_end) {

		eeprom_read(root->pt, (unsigned char*) Buff, sizeof(struct Loggable));
		root->pt += size;
		_PRINTF("*");
		if( ((struct Loggable *)Buff)->state == EMPTY){ // Update report
			_PRINTF("EMPTY => WRITING... ");
			((struct Loggable*)data)->state = USED;
			((struct Loggable*)data)->id = i;
			root->pt -= size;
			eeprom_write(root->pt, (unsigned char *) data, size);
			break;
		}
		else {
			_PRINTF("[USED]");
		}
	i++;
	}

	if(root->pt >= cfs_offset_end){
		_PRINTF(" [FAILED]\n");
		return -1;
	}

	_PRINTF(" [SUCCESS]\n");

return 0;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief      Driver function for deleting in EEPROM memory.
 * \param buf  Buffer with minimal size of sizeof(struct Loggable)
 * \param root File pointer on the EEPROM memory.
 * \param size Size of the dataStructure to store the result.
 * \param id   Id of the loggable data to delete, must be equal to -1 to format all slot.
 * \param cfs_offset Address memory of the first slot
 * \param cfs_offset_end Address memory of the last slot

 * \retval 0   Success in deleting.
 * \retval 1   Fail in deleting.
 *
 */
char file_delete(void * buf, char * Buff, struct File * root, int id, int size, int cfs_offset, int cfs_offset_end){

	_PRINTF("Format :\n");
	((struct Loggable*)buf)->id = 1;

	if(id != -1)
		eeprom_read(CFS_SYSTEM_OFFSET, (unsigned char*) &root, sizeof(struct File));

	else 
		eeprom_write(CFS_SYSTEM_OFFSET, 0, sizeof(struct File));
//_PRINTF("1\n");
	root->pt = cfs_offset;

	while(root->pt <  cfs_offset_end) {
//printf("0-%d|%d(%d)\n",root->pt,cfs_offset_end,size);
		eeprom_read(root->pt, (unsigned char*) Buff, sizeof(struct Loggable));
//printf("1-%d|%d(%d)\n",root->pt,cfs_offset_end,size);
		root->pt += size;

		if(id == -1 || ((struct Loggable *)buf)->id == id){
			_PRINTF("*");
			((struct Loggable*)buf)->state = EMPTY;
//printf("2-%d|%d(%d)\n",root->pt,cfs_offset_end,size);
			root->pt -= size;
//printf("3-%d|%d(%d)\n",root->pt,cfs_offset_end,size);
			eeprom_write(root->pt, (unsigned char *) buf, size);
//printf("4-%d|%d(%d)\n",root->pt,cfs_offset_end,size);
			root->pt += size;
//printf("5-%d|%d(%d)\n",root->pt,cfs_offset_end,size);
			}
//printf("6-%d|%d(%d)\n",root->pt,cfs_offset_end,size);

		((struct Loggable*)buf)->id ++;
	}

	root->pt = cfs_offset;

	_PRINTF(" [SUCCESS]\n");

	return 0;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Driver function for reading in EEPROM memory.
 * \param data DataStructure to store the result.
 * \param type Type of reading : 	CFS_BEGIN, CFS_NEXT, CFS_FIRST_EMPTY
 * \param root File pointer on the EEPROM memory.
 * \param size Size of the dataStructure to store the result.
 * \param id   Id of the loggable data to read, must be equal to -1 to read the next slot.
 * \param cfs_offset Address memory of the first slot
 * \param cfs_offset_end Address memory of the last slot

 * \retval 0   Success in reading.
 * \retval 1   Fail in reading.
 *
 */
char file_read(void * data, char type, struct File * root, int size,int id, int cfs_offset, int cfs_offset_end){

	_PRINTF("READ :");

	if(type == CFS_BEGIN)
		root->pt = cfs_offset;

	if( root->pt >= cfs_offset_end)
		return -1;

	do {
	_PRINTF("*");
	eeprom_read(root->pt, data, size);
	root->pt += size;

	if(id != -1 && ((struct Loggable*)data)->id == id)
		break;

	}
	while ( ( ((struct Loggable*)data)->state == EMPTY || id != -1 ) && root->pt <  cfs_offset_end );

	_PRINTF(" [SUCCESS]\n");

	if( root->pt >= cfs_offset_end)
		return -1;

	return 0;
}

/** @} */
