#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct s_client {
	int id;
	char msg[60000];
} t_client;

t_client clients[FD_SETSIZE];
struct sockaddr_in servaddr;
fd_set set_read, set_write, set_current;
int sockfd, next_id = 0, max_fd = 0;
char buffer_write[70000] = {0};

void handle_error(char *msg)
{
	write(2, msg, strlen(msg));
	exit(1);
}

void send_msg(int curr_fd)
{
	for (int i = 0; i <= max_fd; i++)
	{
		if (i != curr_fd && FD_ISSET(i, &set_write))
		{
			if (send(i, buffer_write, strlen(buffer_write), 0) < 0)
				handle_error("Fatal error\n");
		}
	}
	bzero(buffer_write, sizeof(buffer_write));
}

void create_new_client(int sockFd)
{
	int new_fd = accept(sockfd, 0, 0);
	if (new_fd < 0)
		handle_error("Fatal error\n");
	if (new_fd >= FD_SETSIZE)
	{
		close(new_fd);
		return;
	}
	FD_SET(new_fd, &set_current);
	clients[new_fd].id = next_id;
	bzero(clients[new_fd].msg, sizeof(clients[new_fd].msg));
	if (max_fd < new_fd) 
		max_fd = new_fd;
	sprintf(buffer_write, "server: client %d just arrived\n", next_id);
	send_msg(sockFd);
	next_id++;
}

void read_input_data(int fd, int bytes, char *buffer_read)
{
	for (ssize_t i = 0, j = strlen(clients[fd].msg); i < bytes; i++, j++)
	{
		clients[fd].msg[j] = buffer_read[i];
		if (clients[fd].msg[j] == '\n')
		{
			clients[fd].msg[j] = '\0';
			sprintf(buffer_write, "client %d: %s\n", clients[fd].id, clients[fd].msg);
			send_msg(fd);
			bzero(clients[fd].msg, sizeof(clients[fd].msg));
			j = -1;
		}
	}
}

int main(int c, char **v)
{
	if (c != 2)
		handle_error("Wrong number of arguments\n");
		
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		handle_error("Fatal error\n");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
	servaddr.sin_port = htons(atoi(v[1]));

	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		handle_error("Fatal error\n");
	if (listen(sockfd, 10) != 0)
		handle_error("Fatal error\n");

	FD_ZERO(&set_current);
	FD_SET(sockfd, &set_current);
	max_fd = sockfd;

	while (1)
	{
		set_read = set_write = set_current;
		if (select(max_fd + 1, &set_read, &set_write, 0, 0) < 0)
			handle_error("Fatal error\n");
			
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (FD_ISSET(fd, &set_read))
			{
				if (fd == sockfd)
					create_new_client(fd);
				else
				{
					char buffer_read[50000] = {0};
					ssize_t bytes_read = recv(fd, buffer_read, sizeof(buffer_read), 0);
					if (bytes_read <= 0)
					{
						sprintf(buffer_write, "server: client %d just left\n", clients[fd].id);
						send_msg(fd);
						FD_CLR(fd, &set_current);
						close(fd);
					}
					else
						read_input_data(fd, bytes_read, buffer_read);
					bzero(buffer_read, sizeof(buffer_read));
				}
			}
		}
	}
	return 0;
}
