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
#define BUFFERSIZE 8192
/*** declarations ***/
typedef struct server_state server_state;
typedef struct client_request client_request;
void HttpTokenizer(const char *request);
int handle_client_reqs(server_state *server_state);
int websocket_init(server_state *server_state);
void HandleHeader(server_state *state, char *header, int size);
void http_handle_requestline(server_state *p_state, int client_FD, char *str,
                             client_request *p_request);

struct server_state {
  int server_socket_fd;
};
struct HTTPResponse {
  int Content_Length;
  char Contant_Type;
};
struct HTTPHeader {
  char header_name[1024];
  char *headerValues[1024];
};
struct client_request {
  char HTTP_version[256];
  char request_type[128];
  char path[2048];
  struct HTTPHeader headers[128];
};
/*** globals ***/

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

int websocket_init(server_state *p_state) {
  if ((p_state->server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    die(p_state, "socket");
  struct sockaddr_in addr = {AF_INET, 0x911f, 0};
  int opt = 1;
  setsockopt(p_state->server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
             sizeof(opt));

  if (bind(p_state->server_socket_fd, (struct sockaddr *)&addr, sizeof(addr)) ==
      -1)
    die(p_state, "bind");

  if (listen(p_state->server_socket_fd, 10) == -1)
    die(p_state, "listen");

  return 0;
}

int get_source_file(char *path, size_t path_size, char **p_file_content,
                    int *p_file_size) {
  size_t filePathsz = path_size + sizeof(char) * (4 + 1);
  char *filePath = malloc(filePathsz);
  snprintf(filePath, filePathsz, "src%s", path);
  // getting files
  FILE *p_file = fopen(filePath, "r");
  free(filePath);
  if (p_file == NULL) {
    return -1;
  }
  fseek(p_file, 0L, SEEK_END);
  int file_size = ftell(p_file);
  *p_file_size = file_size;
  fseek(p_file, 0L, SEEK_SET);
  char *fcontent = malloc(file_size);
  fread(fcontent, 1, file_size, p_file);
  *p_file_content = fcontent;
  return 0;
}

void http_write(int client_FD, char *string) {
  int len = strlen(string);
  write(client_FD, string, len);
}
/*** incoming_request ***/
void handle_http_stream(server_state *p_state, int client_FD) {

  char buf[BUFFERSIZE] = {0};
  int bytes_read;
  client_request request;
  client_request *p_request = &request;

  // static allocation for now
  bytes_read = recv(client_FD, buf, BUFFERSIZE - 1, 0);
  buf[bytes_read] = '\0';

  char *saveptr = buf;

  char *token = strtok_r(buf, "\r\n", &saveptr);
  printf("First line of request: %s\n", token);

  http_handle_requestline(p_state, client_FD, strdup(token), p_request);

  while ((token = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
    HandleHeader(p_state, strdup(token), (strlen(token) + 1) * sizeof(char));
  }

  char *f_content;
  int file_size;
  if (get_source_file(p_request->path, sizeof(p_request->path), &f_content,
                      &file_size) == -1)
    die(p_state, "getting file failed");

  http_write(client_FD, "HTTP/1.0 200 OK\r\n");
  char *content_len;
  asprintf(&content_len, "Content-Length: %d\r\n", file_size);
  http_write(client_FD, content_len);
  free(content_len);
  http_write(client_FD, "Connection: close\r\n");
  http_write(client_FD, "Content-Type: text/html\r\n");
  http_write(client_FD, "Accept-Encoding: identity\r\n\r\n");

  write(client_FD, f_content, file_size);
  free(f_content);
  close(client_FD);
  return;
}
