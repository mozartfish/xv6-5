#ifndef PWD_H
#define PWD_H

typedef ushort uid_t;
struct passwd {
  char *pw_name;
  uid_t pw_uid;
};

#endif/*PWD_H*/
