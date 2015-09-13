/////////////////////////////////////////////////////////////////////////////
//
// Mini IoT Server
// Developed by Stanley Huang for Project OpenIoT
// Distributed under GPL v3
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "httppil.h"
#include "httpapi.h"
#include "revision.h"

int uhSerial(UrlHandlerParam* param);
int uhSensors(UrlHandlerParam* param);

UrlHandler urlHandlerList[]={
	{"sensors", uhSensors, NULL },
#ifdef ENABLE_SERIAL
	{"serial", uhSerial, NULL},
#endif
	{NULL},
};

AuthHandler authHandlerList[]={
	{"stats", "user", "pass", "group=admin", ""},
	{NULL}
};

HttpParam httpParam;

extern FILE *fpLog;


//////////////////////////////////////////////////////////////////////////
// callback from the web server whenever it needs to substitute variables
//////////////////////////////////////////////////////////////////////////
int DefaultWebSubstCallback(SubstParam* sp)
{
	// the maximum length of variable value should never exceed the number
	// given by sp->iMaxValueBytes
	if (!strcmp(sp->pchParamName,"mykeyword")) {
		return sprintf(sp->pchParamValue, "%d", 1234);
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////
// callback from the web server whenever it recevies posted data
//////////////////////////////////////////////////////////////////////////
int DefaultWebPostCallback(PostParam* pp)
{
  int iReturn=WEBPOST_OK;

  // by default redirect to config page
  //strcpy(pp->chFilename,"index.htm");

  return iReturn;
}

//////////////////////////////////////////////////////////////////////////
// callback from the web server whenever it receives a multipart
// upload file chunk
//////////////////////////////////////////////////////////////////////////
int DefaultWebFileUploadCallback(HttpMultipart *pxMP, OCTET *poData, size_t dwDataChunkSize)
{
  // Do nothing with the data
	int fd = (int)pxMP->pxCallBackData;
	if (!poData) {
		// to cleanup
		if (fd > 0) {
			close(fd);
			pxMP->pxCallBackData = NULL;
		}
		return 0;
	}
	if (!fd) {
		char filename[256];
		snprintf(filename, sizeof(filename), "%s/%s", httpParam.pchWebPath, pxMP->pchFilename);
		fd = open(filename, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0);
		pxMP->pxCallBackData = (void*)fd;
	}
	if (fd <= 0) return -1;
	write(fd, poData, dwDataChunkSize);
	if (pxMP->oFileuploadStatus & HTTPUPLOAD_LASTCHUNK) {
		close(fd);
		pxMP->pxCallBackData = NULL;
	}
	printf("Received %u bytes for multipart upload file %s\n", dwDataChunkSize, pxMP->pchFilename);
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// callback from the web server whenever it recevies UDP data
//////////////////////////////////////////////////////////////////////////

#define MAX_PIDS 3
float data[MAX_PIDS] = { 0 };

int IncomingUDPCallback(void* _hp)
{
	HttpParam* hp = (HttpParam*)_hp;
	struct sockaddr_in cliaddr;
	socklen_t len = sizeof(cliaddr);
	char buf[1024];
	int n;
	char *s;

	if ((n = recvfrom(hp->udpSocket, buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr, &len)) <= 0)
		return -1;

	buf[n] = 0;
	DBG("Sensor Data: %s\n", buf);

	for (s = strtok(buf, ","), n = 0; s && n < MAX_PIDS; s = strtok(0, ","), n++) {
		data[n] = (float)atof(s);
	}
	return 0;
}

int uhSensors(UrlHandlerParam* param)
{
	DBG("%s\n", param->pucRequest);
	param->fileType = HTTPFILETYPE_JS;
	param->dataBytes = sprintf(param->pucBuffer, "{\r\n\
	\"sensors\":[\r\n\
	{\r\n\
		\"id\":\"%u\",\r\n\
		\"value\":\"%f\"\",\r\n\
		\"time\":\"%u\"\",\r\n\
	},\r\n\
	{\r\n\
		\"id\":\"%u\",\r\n\
		\"value\":\"%f\"\",\r\n\
		\"time\":\"%u\"\",\r\n\
	}\r\n\
	]\r\n}", 1, data[1], (unsigned int)data[0], 2, data[2], (unsigned int)data[0]);
	return FLAG_DATA_RAW;
}

void Shutdown()
{
	//shutdown server
	mwServerShutdown(&httpParam);
	fclose(fpLog);
	UninitSocket();
}

char* GetLocalAddrString()
{
	// get local ip address
	struct sockaddr_in sock;
	char hostname[128];
	struct hostent * lpHost;
	gethostname(hostname, 128);
	lpHost = gethostbyname(hostname);
	memcpy(&(sock.sin_addr), (void*)lpHost->h_addr_list[0], lpHost->h_length);
	return inet_ntoa(sock.sin_addr);
}

int ServerQuit(int arg) {
	static int quitting = 0;
	if (quitting) return 0;
	quitting = 1;
	if (arg) printf("\nCaught signal (%d). Mini IoT Server shutting down...\n",arg);
	Shutdown();
	return 0;
}

void GetFullPath(char* buffer, char* argv0, char* path)
{
	char* p = strrchr(argv0, '/');
	if (!p) p = strrchr(argv0, '\\');
	if (!p) {
		strcpy(buffer, path);
	} else {
		int l = p - argv0 + 1;
		memcpy(buffer, argv0, l);
		strcpy(buffer + l, path);
	}
}

int main(int argc,char* argv[])
{
	fprintf(stderr,"Mini IoT Server (built on %s)\n(C)2005-2015 Written by Stanley Huang <stanleyhuangyc@gmail.com>\n\n", __DATE__);

#ifdef WIN32
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ServerQuit, TRUE );
#else
	signal(SIGINT, (void *) ServerQuit);
	signal(SIGTERM, (void *) ServerQuit);
	signal(SIGPIPE, SIG_IGN);
#endif

	//fill in default settings
	mwInitParam(&httpParam);
	httpParam.maxClients=32;
	httpParam.httpPort = 80;
	GetFullPath(httpParam.pchWebPath, argv[0], "htdocs");
	httpParam.pxAuthHandler = authHandlerList;
	httpParam.pxUrlHandler=urlHandlerList;
	httpParam.flags=FLAG_DIR_LISTING;
	httpParam.tmSocketExpireTime = 15;
	httpParam.pfnPost = DefaultWebPostCallback;
	httpParam.pfnFileUpload = DefaultWebFileUploadCallback;
	httpParam.pfnIncomingUDP = IncomingUDPCallback;

	//parsing command line arguments
	{
		int i;
		for (i=1;i<argc;i++) {
			if (argv[i][0]=='-') {
				switch (argv[i][1]) {
				case 'h':
					fprintf(stderr,"Usage: iotserver	-h	: display this help screen\n"
						       "		-v	: log status/error info\n"
						       "		-p	: specifiy HTTP port [default 80]\n"
						       "		-u	: specifiy UDP port\n"
						       "		-r	: specify http document directory [default htdocs]\n"
						       "		-l	: specify log file\n"
						       "		-m	: specifiy max clients [default 32]\n"
						       "		-M	: specifiy max clients per IP\n"
							   "		-s	: specifiy download speed limit in KB/s [default: none]\n"
							   "		-n	: disallow multi-part download [default: allow]\n"
						       "		-d	: disallow directory listing [default ON]\n\n");
					fflush(stderr);
                                        exit(1);

				case 'p':
					if (++i < argc) httpParam.httpPort=atoi(argv[i]);
					break;
				case 'r':
					if (++i < argc) strncpy(httpParam.pchWebPath, argv[i], sizeof(httpParam.pchWebPath) - 1);
					break;
				case 'l':
					if (++i < argc) fpLog=freopen(argv[i],"w",stderr);
					break;
				case 'm':
					if (++i <argc) httpParam.maxClients=atoi(argv[i]);
					break;
				case 'M':
					if (++i < argc) httpParam.maxClientsPerIP=atoi(argv[i]);
					break;
				case 's':
					if (++i < argc) httpParam.maxDownloadSpeed=atoi(argv[i]);
					break;
				case 'n':
					httpParam.flags |= FLAG_DISABLE_RANGE;
					break;
				case 'd':
					httpParam.flags &= ~FLAG_DIR_LISTING;
					break;
				case 'u':
					if (++i < argc) httpParam.udpPort = atoi(argv[i]);
					break;
				}
			}
		}
	}

	InitSocket();

	printf("Host: %s:%d\n", GetLocalAddrString(), httpParam.httpPort);
	printf("Web root: %s\n",httpParam.pchWebPath);
	printf("Max clients (per IP): %d (%d)\n",httpParam.maxClients, httpParam.maxClientsPerIP);
	if (httpParam.flags & FLAG_DIR_LISTING) printf("Dir listing enabled\n");
	if (httpParam.flags & FLAG_DISABLE_RANGE) printf("Byte-range disabled\n");

	//start server
	if (mwServerStart(&httpParam)) {
		printf("Error starting HTTP server\n");
	} else {
		mwHttpLoop(&httpParam);
	}

	Shutdown();

#ifdef _DEBUG
	getchar();
#endif
	return 0;
}
////////////////////////////// END OF FILE //////////////////////////////
