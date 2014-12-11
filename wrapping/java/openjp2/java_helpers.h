#ifndef _JAVE_HELPERS_H_
#define _JAVE_HELPERS_H_

extern int outfile_format(char *filename);
extern void error_callback(const char *msg, void *client_data);
extern void warning_callback(const char *msg, void *client_data);
extern void info_callback(const char *msg, void *client_data);


#endif /* _JAVE_HELPERS_H_ */
