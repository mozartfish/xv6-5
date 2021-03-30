#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

char *
getenv(char *name)
{
  char **s;
  for(s=environ; *s; ++s) {
    // strcmp up to =
    char *c, *namec=name;
    for (c=*s; *c==*namec && *c != '0'; ++c, ++namec);
    if (*c=='=' && *namec=='\0') {
      // equal
      return c+1;
    }
  }
  return 0;
}
void
setenv(char *name, char *value)
{
  char **s;
  int i;
  for(s=environ, i=0; *s; ++s, ++i) {
    // strcmp up to =
    char *c, *namec=name;
    for (c=*s; *c==*namec && *c != '0'; ++c, ++namec);
    if (*c=='=' && *namec=='\0') {
      // equal, we have to overwrite
      c[1]='\0';
      *s= strcpy(malloc(strlen(*s)+strlen(value)+1), *s);
      strcpy(*s+strlen(*s), value);
      return;
    }
  }
  // have to realloc entire env array 
  s=environ;
  environ=malloc(sizeof(char *)*(i+2));
  environ[0]=strcpy(malloc(strlen(name)+strlen(value)+2), name);
  environ[0][strlen(name)]='=';
  strcpy(environ[0]+strlen(name)+1, value);
  for(i=1; s[i-1]; ++i) {
    environ[i]=s[i-1];
  }
  environ[i]=0;
  return;
}
