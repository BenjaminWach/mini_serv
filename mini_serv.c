#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct s_clients{
	int id;
	char msg[300000];
} t_clients;

t_clients client[1024];
char buffer_read[300000] = {0};
char buffer_write[300020] = {0};

int sockfd;
int next_id = 0;
int max_fd = 0;

struct sockaddr_in servaddr;
fd_set set_read, set_write, set_current;

void handle_error(char *msg)
{
	write(2, msg, strlen(msg));
	exit(1);
}

void send_msg(int curr_fd)
{
	for (int i = 0; i <= max_fd ; i++)
	{
		if (i != curr_fd && FD_ISSET(i, &set_write))
		{
			if (send(i, buffer_write, strlen(buffer_write), 0) < 0){
				handle_error("Fatal error\n");}
		}
	}
	bzero(buffer_write, sizeof(buffer_write));
}

int main(int ac, char **av)
{
	if (ac != 2){
		handle_error("Wrong number of arguments\n");}
	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1){
		handle_error("Fatal error\n");}
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0){
		handle_error("Fatal error\n");}
	if (listen(sockfd, 10) != 0){
		handle_error("Fatal error\n");}

	FD_ZERO(&set_current);
	FD_ZERO(&set_read);
	FD_ZERO(&set_write);

	FD_SET(sockfd, &set_current);
	max_fd = sockfd;

	while (1)
	{
		set_read = set_write = set_current;

		if (select(max_fd + 1, &set_read, &set_write, 0, 0) < 0){
			handle_error("Fatal error\n");}
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (FD_ISSET(fd, &set_read))
			{
				if (fd == sockfd)
				{
					int new_fd = accept(sockfd, 0, 0);
					if (new_fd < 0)
						handle_error("Fatal error\n");
					client[new_fd].id = next_id;
					bzero(client[new_fd].msg, sizeof(client[new_fd].msg));
					FD_SET(new_fd, &set_current);
					if (max_fd < new_fd)
						max_fd = new_fd;
					sprintf(buffer_write, "server: client %d just arrived\n", next_id);
					send_msg(fd);
					next_id++;
				}
				else
				{
					bzero(buffer_read, sizeof(buffer_read));
					ssize_t bytes_read = recv(fd, buffer_read, sizeof(buffer_read), 0);
					if (bytes_read <= 0)
					{
						sprintf(buffer_write, "server: client %d just left\n", client[fd].id);
						send_msg(fd);
						FD_CLR(fd, &set_current);
						close(fd);
					}
					else
					{
						int msg_len = strlen(client[fd].msg);
						for (ssize_t i = 0; i < bytes_read; i++)
						{
							client[fd].msg[msg_len++] = buffer_read[i];
							
							if (buffer_read[i] == '\n')
							{
								client[fd].msg[msg_len - 1] = '\0';
								sprintf(buffer_write, "client %d: %s\n", client[fd].id, client[fd].msg);
								send_msg(fd);
								bzero(client[fd].msg, sizeof(client[fd].msg));
								msg_len = 0;
							}
						}
					}
				}
			}
		}
	}
}
