#ifndef SERVER_H
#define SERVER_H

#define BUFFER_REQUEST_SIZE 1024
#define BUFFER_RESPONSE_SIZE 4096
#define MAX_CONNECTIONS 1000
#define KEEP_ALIVE_TIMEOUT 30 // seconds

// Keep Alive info
// https://www.ibm.com/docs/es/was/8.5.5?topic=stores-tuning-detection-database-connection-loss
// https://datatracker.ietf.org/doc/html/draft-thomson-hybi-http-timeout-03#section-2
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Keep-Alive
// https://www.rfc-editor.org/rfc/rfc2068

// tcp_keepalive_probes: the number of unacknowledged probes to send before considering the connection dead and notifying the application layer
#define KEEP_ALIVE_TCP_KEEPCNT 20
// tcp_keepalive_time: the interval between the last data packet sent (simple ACKs are not considered data) and the first keepalive probe; 
// after the connection is marked to need keepalive, this counter is not used any further
#define KEEP_ALIVE_TCP_KEEPIDLE 700 
// tcp_keepalive_intvl: the interval between subsequential keepalive probes, regardless of what the connection has exchanged in the meantime
#define KEEP_ALIVE_TCP_KEEPINTVL 10 



#include "options.h" // for struct Options
#include <signal.h>     // for sigaction
#include <netinet/tcp.h> // for SOL_TCP and KeepAlive


volatile sig_atomic_t sigintReceived;

void serverRun(struct Options options);

int makeSocketNonBlocking(int sfd);
void makeKeepAlive(int socketFd);

#endif // SERVER_H