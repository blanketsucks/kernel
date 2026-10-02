#include <pwd.h>

extern "C" {

static struct passwd __root_passwd = {
    .pw_name = const_cast<char*>("root"),
    .pw_uid = 0,
    .pw_gid = 0,
    .pw_dir = const_cast<char*>("/home"),
    .pw_shell = const_cast<char*>("/bin/shell")
};

struct passwd* getpwnam(const char*) {
    return &__root_passwd;
}

struct passwd* getpwuid(uid_t) {
    return &__root_passwd;
}

struct passwd* getpwent() {
    return &__root_passwd;
}

void endpwent() {}

}