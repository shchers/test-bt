#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h> // Just for sleep funciton
#include "sserial.h"

#if 0
void SleepMs(unsigned nDelayMs) {
	struct timespec trest;
	struct timespec tneed;
	tneed.tv_sec = 0;
	tneed.tv_nsec = nDelayMs * 1000000;
	nanosleep(&tneed, &trest);
}
#else
#define SleepMs(nDelayMs) usleep(nDelayMs*1000)
#endif

/**
 * Pinout:
 *  nReset    -> nRTS (out)
 *  AT/nData  -> nDTR (out)
 *  Connected -> nCTS (in)
 */

void resetBluetooth(struct sserial_props *pProps) {
	SetRts(pProps);
	SleepMs(100);
	ClrRts(pProps);
	SleepMs(10);
}

void initBluetooth(struct sserial_props *pProps) {
	fprintf(stderr, "> Triger reset pin\n");
	resetBluetooth(pProps);

	fprintf(stderr, "> Switch device to AT-mode\n");
	ClrDtr(pProps);
	SleepMs(1000);

#if 0
	fprintf(stderr, "> Reset to default\n");
	WritePort(pProps, "AT+ORGL" CRLF, strlen("AT+ORGL" CRLF));
	SleepMs(1000);
#endif

	fprintf(stderr, "> Switch to Slave mode\n");
	WritePort(pProps, "AT+ROLE=0" CRLF, strlen("AT+ROLE=0" CRLF));
	SleepMs(1000);

	fprintf(stderr, "> Set name\n");
	WritePort(pProps, "AT+NAME=ww-test" CRLF, strlen("AT+NAME=qq-test" CRLF));
	SleepMs(1000);

	fprintf(stderr, "> Set password\n");
	WritePort(pProps, "AT+PSWD=1237" CRLF, strlen("AT+PSWD=1235" CRLF));
	SleepMs(100);

	fprintf(stderr, "> Switch device to Data-mode\n");
	SetDtr(pProps);
	SleepMs(10);

	resetBluetooth(pProps);
}

int main(void) {
	struct sserial_props *pProps = OpenPort("/dev/ttyUSB0", 38400);
	if (pProps == NULL) {
		fprintf(stderr, "Port init failed\n");
		return 1;
	}

	initBluetooth(pProps);

	char ch='\0';
	// print all until "a" will be be recieved
	while (ch!='a') {
		int bDisconnected = GetCts(pProps);
		// It will be true until someone connected
		if (bDisconnected != 0) {
			continue;
		}

		// skip empty and line endings
		if (ReadPort(pProps, &ch, 1) <= 0 ||
				ch == '\r' || ch == '\n')
			continue;
		printf(">> %c\n", ch);
	}

	ClosePort(pProps);
	return 0;
}
