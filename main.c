#include <asm-generic/socket.h>
#include <bits/types/__sigval_t.h>
#include <bits/types/siginfo_t.h>
#include <ctype.h>
#include <netinet/in.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/*** defines ***/
/*** declarations ***/
typedef struct server_state server_state;
typedef struct client_request client_request;
struct server_state {
  int server_socket_fd;
};
struct HTTPResponse {
  int Content_Length;
  char Contant_Type;
};
void cleanup(server_state *state) {
  close(state->server_socket_fd);
  return;
}
void die(server_state *state, const char *err) {
  perror(err);
  cleanup(state);
  exit(1);
}

/*** init ***/
int main() {
  server_state p_state;
  p_state.server_socket_fd = -1;

  websocket_init(&p_state);
  handle_client_reqs(&p_state);
  cleanup(&p_state);

  return 0;
}
