#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define CONFIG_FILE "client.txt"

// Here is an example of the config file
// server_ip=100.101.164.159
// server_port=5000

// The reason for having a configuration file is to allow
// multiple clients to use the same server.
// Each client will be able to write to the client.txt file
// in order to get its keystrokes.

void read_config(char *server_ip, int *server_port)
{
    /* open config file */
    FILE *fp = fopen(CONFIG_FILE, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Cannot open config file\n");
        return;
    }

    /* read server IP and port from file */
    char line[100];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (sscanf(line, "server_ip=%s", server_ip) == 1)
        {
            continue;
        }
        if (sscanf(line, "server_port=%d", server_port) == 1)
        {
            continue;
        }
    }

    /* close config file */
    fclose(fp);
}

int validate_config(char *server_ip, int server_port)
{
    /* validate server IP */
    struct in_addr addr;
    if (inet_pton(AF_INET, server_ip, &addr) != 1)
    {
        fprintf(stderr, "Invalid server IP: %s\n", server_ip);
        return 0;
    }

    /* validate server port */
    if (server_port < 1 || server_port > 65535)
    {
        fprintf(stderr, "Invalid server port: %d\n", server_port);
        return 0;
    }

    return 1;
}

void send_keyevent_udp(unsigned int keycode, int event_type)
{
    /* read config file */
    char server_ip[100] = "";
    int server_port = 0;
    read_config(server_ip, &server_port);

    /* if config file not found, do not send key event */
    if (server_ip[0] == '\0' || server_port == 0)
    {
        return;
    }

    /* validate server IP and port */
    if (!validate_config(server_ip, server_port))
    {
        return;
    }

    /* create UDP socket */
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "Cannot create UDP socket\n");
        exit(1);
    }

    /* set server address */
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(server_ip);
    servaddr.sin_port = htons(server_port);

    /* send key event to server */
    char buf[100];
    snprintf(buf, sizeof(buf), "%x,%d", keycode, event_type);
    sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    /* cleanup */
    close(sockfd);
}

void event_loop_x11()
{
    Display *display;
    Window window;
    XEvent event;

    /* open connection with the server */
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    /* create window */
    int s = DefaultScreen(display);
    window = XCreateSimpleWindow(display, RootWindow(display, s), 10, 10, 200, 200, 1,
        BlackPixel(display, s), WhitePixel(display, s));

    /* select all key events */
    XSelectInput(display, window, KeyPressMask | KeyReleaseMask);

    /* map (show) the window */
    XMapWindow(display, window);

    /* event loop */
    while (1)
    {
        XNextEvent(display, &event);

        /* keyboard events */
        if (event.type == KeyPress)
        {
            printf("KeyPress: %x\n", event.xkey.keycode);

            /* send key press event to server */
            send_keyevent_udp(event.xkey.keycode, 0);
        }
        else if (event.type == KeyRelease)
        {
            printf("KeyRelease: %x\n", event.xkey.keycode);

            /* send key release event to server */
            send_keyevent_udp(event.xkey.keycode, 1);
        }
    }

    /* cleanup */
    XCloseDisplay(display);
}

int main()
{
    /* start event loop */
    event_loop_x11();

    return 0;
}