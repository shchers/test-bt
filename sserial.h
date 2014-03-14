/**
 * Simple serial port module
 * (c) Sergey Shcherbakov <shchers@gmail.com>
 */

#include <termios.h>
#include <pthread.h>

#ifndef SSERIAL_H
#define SSERIAL_H

#ifndef CRLF
#define CRLF "\x0d\x0a"
#endif

struct sserial_props {
	/// Port file descriptor
	int fd;
	/// Storage of the original terminal properties
	struct termios otinfo;
	/// Synchronization mutex
	pthread_mutex_t mtx;
};

struct sserial_props *OpenPort(const char *portName);
void ClosePort(struct sserial_props *pProps);
int UpdateRts(struct sserial_props *pProps, int bSet);
#define SetRts(pProps) UpdateRts(pProps, 1)
#define ClrRts(pProps) UpdateRts(pProps, 0)
int UpdateDtr(struct sserial_props *pProps, int bSet);
#define SetDtr(pProps) UpdateDtr(pProps, 1)
#define ClrDtr(pProps) UpdateDtr(pProps, 0)

#endif // SSERIAL_H
