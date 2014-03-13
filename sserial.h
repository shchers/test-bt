/**
 * Simple serial port module
 * (c) Sergey Shcherbakov <shchers@gmail.com>
 */

#ifndef CRLF
#define CRLF "\x0d\x0a"
#endif

int OpenPort(const char *portName);
void ClosePort(int fd);
int updateRts(int fd, int bSet);
#define setRts(fd) updateRts(fd, 1)
#define clrRts(fd) updateRts(fd, 0)
int updateDtr(int fd, int bSet);
#define setDtr(fd) updateDtr(fd, 1)
#define clrDtr(fd) updateDtr(fd, 0)
