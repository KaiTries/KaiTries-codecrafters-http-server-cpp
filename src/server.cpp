#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char **argv)
{
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  // // Since the tester restarts your program quite often, setting REUSE_PORT
  // // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  //
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  //
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0)
  {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  // accept a new connection, return socket file descriptor of client
  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
  std::cout << "Client connected\n";



  // read the request from the client
  char buffer[1024] = {0};
  read(client_fd, buffer, 1024);
  std::cout << "Request: " << buffer << std::endl;

  // only get the first line of the request
  std::string request(buffer);
  std::string first_line = request.substr(0, request.find("\r\n"));
  std::string path = first_line.substr(first_line.find(" ") + 1, first_line.rfind(" ") - first_line.find(" ") - 1);


  std::string response;
  if (path == "/")
  {
    // response string to the client
    response = "HTTP/1.1 200 OK\r\n\r\n";
  } else if (/*path starts with /echo/*/ path.find("/echo") == 0){
    // response string to the client
    std::string echo = path.substr(path.find(" ") + 6, path.rfind(" ") - path.find(" ") - 1);
    std::string len_str = std::to_string(echo.length()); // Convert len to string
    response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + len_str + "\r\n\r\n" + echo + "\r\n\r\n";
  
  } else if (path.find("/user-agent") == 0) {
    // find the line that starts with User-Agent:
    std::string user_agent = "";
    size_t pos = request.find("User-Agent: ");
    if (pos != std::string::npos) {
      size_t end = request.find("\r\n", pos);
      user_agent = request.substr(pos + 12, end - pos - 12);
    }
    std::string len_str = std::to_string(user_agent.length()); // Convert len to string
    response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + len_str + "\r\n\r\n" + user_agent + "\r\n\r\n";

  }
  else {
    // response string to the client
    response = "HTTP/1.1 404 Not Found\r\n\r\n";
  }


  // send the response to the client
  send(client_fd, response.c_str(), response.size(), 0);


  close(server_fd);

  return 0;
}
