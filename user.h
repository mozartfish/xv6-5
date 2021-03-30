struct stat;
struct rtcdate;

extern char **environ;
typedef ushort uid_t;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int execve(char*, char**, char**);
int open(const char*, int, ...);
int mknod(const char*, uid_t, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*, uid_t);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int chown(const char*, uid_t);
int chmod(const char*, int);
int getuid(void);
int setuid(int uid);
int stat(const char*, struct stat*);

// ulib.c
int execv(char*, char**);
char *strtok(char*,char*);
//int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);

// uenv.c
char *getenv(char*);
void setenv(char*, char*);

// upasswd.c
struct passwd *getpwent(void);
void setpwent(void);
struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);
