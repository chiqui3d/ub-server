#ifndef SERVER_H
#define SERVER_H

#define BUFFER_REQUEST_SIZE 1024
#define BUFFER_RESPONSE_SIZE 4096
#define MAX_CONNECTIONS 1000
#define KEEP_ALIVE_TIMEOUT 60 // seconds

// TCP Keep Alive, TCP and HTTP keep-alive are different
// https://stackoverflow.com/questions/411460/use-http-keep-alive-for-server-to-communicate-to-client
// https://www.ibm.com/docs/es/was/8.5.5?topic=stores-tuning-detection-database-connection-loss
// https://datatracker.ietf.org/doc/html/draft-thomson-hybi-http-timeout-03#section-2
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Keep-Alive
// https://www.rfc-editor.org/rfc/rfc2068
// https://www.rfc-editor.org/rfc/rfc2616.html#page-44

// tcp_keepalive_probes (number): the number of unacknowledged probes to send before considering the connection dead
#define KEEP_ALIVE_TCP_KEEPCNT 20
// tcp_keepalive_time (seconds): the wait interval before send the first keepalive probe; 
// If no ACK response is received for KEEP_ALIVE_TCP_KEEPCNT consecutive times, the connection is marked as broken.
#define KEEP_ALIVE_TCP_KEEPIDLE 700 
// tcp_keepalive_intvl (seconds): the interval between subsequential keepalive probes (resend) after the first probe of KEEP_ALIVE_TCP_KEEPIDLE seconds
#define KEEP_ALIVE_TCP_KEEPINTVL 10 



#include "options.h" // for struct Options
#include <signal.h>     // for sigaction
#include <netinet/tcp.h> // for SOL_TCP and KeepAlive


volatile sig_atomic_t sigintReceived;

void serverRun(struct Options options);

int makeSocketNonBlocking(int sfd);
void makeTCPKeepAlive(int socketFd);

#endif // SERVER_H