/**
 * Simple serial port module
 * (c) Sergey Shcherbakov <shchers@gmail.com>
 */

#include <termios.h>
#include <pthread.h>
#include <unistd.h>

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
	/// Synchronization mutex for control pins
	pthread_mutex_t mtx_ctrl;
	/// Synchronization mutex for Rx pipe
	pthread_mutex_t mtx_rx;
	/// Synchronization mutex for Tx pipe
	pthread_mutex_t mtx_tx;
};

struct sserial_props *OpenPort(const char *portName, int nBaudrate);
void ClosePort(struct sserial_props *pProps);
int UpdateRts(struct sserial_props *pProps, int bSet);
#define SetRts(pProps) UpdateRts(pProps, 1)
#define ClrRts(pProps) UpdateRts(pProps, 0)
int UpdateDtr(struct sserial_props *pProps, int bSet);
#define SetDtr(pProps) UpdateDtr(pProps, 1)
#define ClrDtr(pProps) UpdateDtr(pProps, 0)
int GetCts(struct sserial_props *pProps);
ssize_t ReadPort(struct sserial_props *pProps, void *pBuff, size_t nSize);
ssize_t WritePort(struct sserial_props *pProps, void *pBuff, size_t nSize);

#endif // SSERIAL_H
