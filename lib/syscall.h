#pragma once

#include "lib/stdint.h"
#include "thread/thread.h"

namespace Systemcall
{
    pid_t getpid();
    // size_t            write(int32_t fd, const void* buf, uint32_t count);
    uint32_t          write(const char* str);
    void*             malloc(uint32_t size);
    void              free(void* p);
    pid_t             fork();
    int32_t           read(int32_t fd, void* buf, uint32_t count);
    void              putchar(char char_asci);
    void              clear(void);
    char*             getcwd(char* buf, uint32_t size);
    int32_t           open(char* pathname, uint8_t flag);
    int32_t           close(int32_t fd);
    int32_t           lseek(int32_t fd, int32_t offset, uint8_t whence);
    int32_t           unlink(const char* pathname);
    int32_t           mkdir(const char* pathname);
    struct dir*       opendir(const char* name);
    int32_t           closedir(struct dir* dir);
    int32_t           rmdir(const char* pathname);
    struct dir_entry* readdir(struct dir* dir);
    void              rewinddir(struct dir* dir);
    // int32_t           stat(const char* path, struct stat* buf);
    int32_t chdir(const char* path);
    void    ps(void);
    int32_t execv(const char* pathname, char** argv);
    void    exit(int32_t status);
    pid_t   wait(int32_t* status);
    int32_t pipe(int32_t pipefd[2]);
    void    fd_redirect(uint32_t old_local_fd, uint32_t new_local_fd);
    void    help(void);
    void    init();
    void    yield();
    void    sleep(uint32_t m_interval);
}  // namespace Systemcall