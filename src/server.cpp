#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <map>     // Add this line
#include <sstream> // Add this line
#include <thread>
#include <algorithm>
#include <cctype>

std::string directory;


struct HttpRequest
{
  std::string method;
  std::string path;
  std::string version;
  std::map<std::string, std::string> headers;
  std::string body;
};

HttpRequest parse_request(const std::string &request)
{
  HttpRequest req;

  std::istringstream request_stream(request);
  std::string line;

  std::getline(request_stream, line);
  std::istringstream request_line_stream(line);
  request_line_stream >> req.method >> req.path >> req.version;

  while (std::getline(request_stream, line) && line != "\r")
  {
    std::istringstream header_line_stream(line);
    std::string header_name;
    std::getline(header_line_stream, header_name, ':');
    std::transform(header_name.begin(), header_name.end(), header_name.begin(),
                    [](unsigned char c){return std::tolower(c);});
    std::string header_value;
    std::getline(header_line_stream, header_value);
    req.headers[header_name] = header_value.substr(1); // Skip the leading space
  }
  std::getline(request_stream, req.body, '\0');
  return req;
}

void handle_client(int client_id)
{
  char buffer[1024] = {0};
  read(client_id, buffer, 1024);
  HttpRequest request = parse_request(buffer);

  std::string path = request.path;

  std::string response;
  if (path == "/")
  {
    // response string to the client
    response = "HTTP/1.1 200 OK\r\n\r\n";
  }
  else if (/*path starts with /echo/*/ path.find("/echo") == 0)
  {
    // response string to the client
    std::string echo = path.substr(path.find(" ") + 7, path.rfind(" ") - path.find(" ") - 1);

    // get the content-encoding header
    std::string encodingHeader = request.headers["accept-encoding"];
    std::cout << "encoding header: " << encodingHeader << std::endl;
    std::string len_str = std::to_string(echo.length()); // Convert len to string
    response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + len_str + "\r\n\r\n" + echo + "\r\n\r\n";
  }
  else if (path.find("/user-agent") == 0)
  {
    std::string user_agent = request.headers["User-Agent"];
    std::string len_str = std::to_string(user_agent.length() - 1); // Convert len to string
    response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + len_str + "\r\n\r\n" + user_agent + "\r\n\r\n";
  }
  else if (path.find("/files") == 0)
  {
    if (request.method == "GET")
    {
      // get file from file path
      std::string file_path = directory + path.substr(7);
      FILE *file = fopen(file_path.c_str(), "r");
      if (file == NULL)
      {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
      }
      else
      {
        std::string content_type = "Content-Type: application/octet-stream\r\n";

        // read out file into body
        std::string body;
        char c;
        while ((c = fgetc(file)) != EOF)
        {
          body += c;
        }
        fclose(file);

        // build response
        response = "HTTP/1.1 200 OK\r\n" + content_type + "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
      }
    }
    else if (request.method == "POST")
    {
      // write contents of body to file specified
      std::string file_path = directory + path.substr(7);
      FILE *file = fopen(file_path.c_str(), "w");

      if (file == NULL)
      {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
      }
      else
      {
        std::cout << request.body << std::endl;
        fwrite(request.body.c_str(), 1, request.body.size(), file);
        fclose(file);
        response = "HTTP/1.1 201 CREATED\r\n\r\n";
      }

    }
  }
  else
  {
    // response string to the client
    response = "HTTP/1.1 404 Not Found\r\n\r\n";
  }


  // send the response to the client
  send(client_id, response.c_str(), response.size(), 0);
  close(client_id);
}

int main(int argc, char **argv)
{
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Loop over the arguments
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];

    // Check if the argument is the --directory flag
    if (arg == "--directory")
    {
      // Make sure we won't go out of bounds
      if (i + 1 < argc)
      {
        // The next argument is the directory
        directory = argv[++i];
      }
      else
      {
        std::cerr << "--directory requires one argument.\n";
        return 1;
      }
    }
  }

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
  while (true)
  {
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    if (client_fd < 0)
    {
      std::cerr << "accept failed\n";
      return 1;
    }
    std::thread client_thread(handle_client, client_fd);
    client_thread.detach();
  }

  return 0;
}
