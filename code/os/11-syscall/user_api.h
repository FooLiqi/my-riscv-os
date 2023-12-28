#ifndef __USER_API_H__
#define __USER_API_H__

/* user mode syscall APIs */
extern int gethid(unsigned int *hid);
extern int gettime(unsigned int *time);
extern void myprintf(const char *s, const char);
extern void myprintfWithoutArgument(const char *s);
extern int compileCode(const char *code);

#endif /* __USER_API_H__ */
