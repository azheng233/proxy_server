#ifndef _BUILD_H_
#define _BUILD_H_

#define SERVER_VERSION "1.2.2"

extern char build_info[];

#define print_buildinfo() \
    printf("compiler version: "__VERSION__"\ndefinitions: %s\n", build_info)

#endif
