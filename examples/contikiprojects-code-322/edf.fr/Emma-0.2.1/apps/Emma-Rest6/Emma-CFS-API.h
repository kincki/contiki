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
 * \defgroup cfs E.M.M.A Rest6 CFS
 * \brief The E.M.M.A Rest6 CFS is a simple and light implementation for writing, deleting and updating data in the EEPROM memory.
 *
 * E.M.M.A Rest6 CFS allows to define several light API for persistent data saving like log or report. The object to save must implement LOGGABLE() structure like Report or Ressource. The child API can be make by using simple define (LogAPI, ReportAPI, ...).  
 * \image html Memory.png
 * @{
 */

/**
 * \file
 *         Header file of EMMA-Rest6 CFS 
 * \author
 *         Clement DUHART <duhart.clement@gmail.com>
 * 
 * 
 *
 */

#ifndef __HEADER_REST6ENGINE_CFS__
#define __HEADER_REST6ENGINE_CFS__
enum {
	EMPTY,
	USED
};


enum{ 
	CFS_BEGIN,
	CFS_NEXT,
	CFS_FIRST_EMPTY
	};


struct File{
	eeprom_addr_t pt;
};

struct Loggable {
	void * next;
	unsigned int id; 
	char state;
	int time;
};

#define CFS_SYSTEM_OFFSET 12
#define LOGGABLE()	void * next; \
										unsigned int id; \
										char state; \
										int time

/*----------------------------------------------------------------------------
 * Driver for all File system
/*----------------------------------------------------------------------------*/
char file_new 		(void * data, char * Buff, 	struct File * root, int size, int cfs_offset, int cfs_offset_end);
char file_update	(void * data, char * Buff, 	struct File * root, int size, int cfs_offset, int cfs_offset_end);
char file_delete	(void * buf,  char* Buff2,	struct File * root, int id, int size, int cfs_offset, int cfs_offset_end);
char file_read		(void * data, char type, 	struct File * root, int size, int id, int cfs_offset, int cfs_offset_end);

/*----------------------------------------------------------------------------
 * File system for report notify
/*----------------------------------------------------------------------------*/

/**
 * \brief      Number of report slot.
 */
#define CFS_REPORT_COUNT 4
#define CFS_REPORT_OFFSET ( CFS_SYSTEM_OFFSET + sizeof (struct File) )
#define CFS_REPORT_OFFSET_END ( CFS_REPORT_OFFSET + ( CFS_REPORT_COUNT * sizeof(struct Report) ) )
/**
 * \brief      Save a new report for notification in the first empty slot in EEPROM memory.
 * \param data Pointer on a SubscriberData object to save.
 * \param buf  An empty buffer with a minimal size of : sizeof(struct Loggable )
 * \retval 0   Success in saving the new report
 * \retval -1  All the report slot are used.
 */
#define REPORT_NEW(data, buf) 			file_new 		(data,buf, &conn->root, sizeof(struct Report), CFS_REPORT_OFFSET, CFS_REPORT_OFFSET_END)
/**
 * \brief      Update a report for notification in EEPROM memory.
 * \param data Pointer on the SubscriberData object to update.
 * \param buf  An empty buffer with a minimal size of : sizeof(struct Loggable )
 * \retval 0   Success in updating the report
 * \retval -1  Unable to find the report in EEPROM memory.
 */
#define REPORT_UPDATE(data, buf) 		file_update (data,buf, &conn->root, sizeof(struct Report), CFS_REPORT_OFFSET, CFS_REPORT_OFFSET_END)
/**
 * \brief      Read the next report in the EEPROM memory.
 * \param out  Pointer on the SubscriberData object to output.
 * \param type Type of reading : 	CFS_BEGIN, CFS_NEXT, CFS_FIRST_EMPTY
 * \retval 0   Success in reading the next report
 * \retval -1  Unable to read the next report in EEPROM memory.
 */
#define REPORT_READ(out, type) 			file_read		(out, type, &conn->root, sizeof(struct Report), -1, CFS_REPORT_OFFSET, CFS_REPORT_OFFSET_END)
/**
 * \brief      Read a specific report in the EEPROM memory.
 * \param out  Pointer on the SubscriberData object to output.
 * \param id	 Id of the report to read.
 * \retval 0   Success in reading the next report
 * \retval -1  Unable to find the report in EEPROM memory.
 */
#define REPORT_OPEN(out, id) 				file_read		(out, CFS_BEGIN, &conn->root, sizeof(struct Report), id, CFS_REPORT_OFFSET, CFS_REPORT_OFFSET_END)
/**
 * \brief      Format all the report slot in EEPROM memory.
 * \param buf  Buffer with minimal size of : sizeof(struct Loggable)
 * \param buff2 Buffer with minimal size of : sizeof(struct Loggable)
 * \retval 0   Success in formating report slot.
 * \retval -1  Error in formating report slot.
 */
#define REPORT_FORMAT(buf,buff2) 		file_delete	(buf,buff2,&conn->root, -1,sizeof(struct Report), CFS_REPORT_OFFSET, CFS_REPORT_OFFSET_END)
/**
 * \brief      Delete a specific report.
 * \param buf  Buffer with minimal size of : sizeof(struct Loggable)
 * \param buff2 Buffer with minimal size of : sizeof(struct Loggable)
 * \param id   The id of the report to remove.
 * \retval 0   Success in deleting the specific report slot.
 * \retval -1  Unable to find the specific report slot.
 */
#define REPORT_DELETE(buf,buff2,id) file_delete	(buf,buff2, &conn->root, id,sizeof(struct Report), CFS_REPORT_OFFSET, CFS_REPORT_OFFSET_END)


/*----------------------------------------------------------------------------
 * File system for data log
/*----------------------------------------------------------------------------*/

/**
 * \brief      Number of log slot.
 */
#define CFS_LOG_COUNT 20
#define CFS_LOG_OFFSET CFS_REPORT_OFFSET_END
#define CFS_LOG_OFFSET_END ( CFS_LOG_OFFSET + ( CFS_LOG_COUNT * sizeof(struct Ressource) ) )

/**
 * \brief      Save a new log in the first empty slot in EEPROM memory found since cfs.
 * \param data Pointer on a Ressource object to save.
 * \param buf  An empty buffer with a minimal size of : sizeof(struct Loggable )
 * \param cfs  File pointer for offset among log slot.
 * \retval 0   Success in saving the new log
 * \retval -1  All the log slot are used.
 */
#define LOG_NEW(data, buf,cfs) 				file_new 		(data,buf, cfs, sizeof(struct Ressource), CFS_LOG_OFFSET, CFS_LOG_OFFSET_END)
/**
 * \brief      Update a log in EEPROM memory.
 * \param data Pointer on the Ressource object to update.
 * \param buf  An empty buffer with a minimal size of : sizeof(struct Loggable )
 * \param cfs  File pointer for offset among log slot.
 * \retval 0   Success in updating the log
 * \retval -1  Unable to find the log in EEPROM memory.
 */
#define LOG_UPDATE(data, buf,cfs) 		file_update (data,buf, cfs, sizeof(struct Ressource), CFS_LOG_OFFSET, CFS_LOG_OFFSET_END)
/**
 * \brief      Read the next log in the EEPROM memory.
 * \param out  Pointer on the Ressource object to output.
 * \param type Type of reading : 	CFS_BEGIN, CFS_NEXT, CFS_FIRST_EMPTY
 * \param cfs  File pointer for offset among log slot.
 * \retval 0   Success in reading the next log
 * \retval -1  Unable to read the next log in EEPROM memory.
 */
#define LOG_READ(out, type,cfs) 			file_read		(out, type, cfs, sizeof(struct Ressource), -1, CFS_LOG_OFFSET, CFS_LOG_OFFSET_END)
/**
 * \brief      Read a specific log in the EEPROM memory.
 * \param out  Pointer on the Ressource object to output.
 * \param id	 Id of the log to read.
 * \param cfs  File pointer for offset among log slot.
 * \retval 0   Success in reading the next log
 * \retval -1  Unable to find the log in EEPROM memory.
 */
#define LOG_OPEN(out, id,cfs) 				file_read		(out, CFS_BEGIN, cfs, sizeof(struct Ressource), id, CFS_LOG_OFFSET, CFS_LOG_OFFSET_END)
/**
 * \brief      Format all the log slot.
 * \param buf  Buffer with minimal size of : sizeof(struct Loggable)
 * \param buff2 Buffer with minimal size of : sizeof(struct Loggable)
 * \param cfs  File pointer for offset among log slot.
 * \retval 0   Success in formating log slot.
 * \retval -1  Unable to format the log slot.
 */
#define LOG_FORMAT(buf,buff2,cfs)			file_delete	(buf,buff2, cfs, -1,sizeof(struct Ressource), CFS_LOG_OFFSET, CFS_LOG_OFFSET_END)
/**
 * \brief      Delete a specific log.
 * \param buf  Buffer with minimal size of : sizeof(struct Loggable)
 * \param buff2 Buffer with minimal size of : sizeof(struct Loggable)
 * \param cfs  File pointer for offset among log slot.
 * \param id   The id of the log to remove.
 * \retval 0   Success in deleting the specific log slot.
 * \retval -1  Unable to find the specific log slot.
 */
#define LOG_DELETE(buf,buff2,cfs,id)	file_delete	(buf,buff2, cfs, id,sizeof(struct Ressource), CFS_LOG_OFFSET, CFS_LOG_OFFSET_END)


#endif

/** @} */
