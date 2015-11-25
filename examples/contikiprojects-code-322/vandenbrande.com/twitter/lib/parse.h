#ifndef __PARSE_TWTR_H_
#define __PARSE_TWTR_H_

int findStatus(const char *buffer, int (*statusAction)(char* sender, char* status), void *context);

#endif // __PARSE_TWTR_H_
