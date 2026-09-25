#pragma once

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stddef.h>

__BEGIN_DECLS

struct passwd { 
    char* pw_name;
    uid_t pw_uid;
    gid_t pw_gid;
    char* pw_dir;
    char* pw_shell;
};

struct passwd* getpwnam(const char* name);
struct passwd* getpwuid(uid_t uid);

int getpwnam_r(const char* name, struct passwd* pwd,
               char* buf, size_t buflen,
               struct passwd** result);
int getpwnuid_r(uid_t uid, struct passwd* pwd,
               char* buf, size_t buflen,
               struct passwd** result);

struct passwd *getpwent(void);
void setpwent(void);
void endpwent(void);

__END_DECLS