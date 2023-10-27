#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// In the future, one shall be able to connect to this computer and register a callback
// instead

// voiceboxclient (work)
#define SERVER_IP "100.101.164.159"
#define SERVER_PORT 5000

void send_keyevent_udp(unsigned int keycode, int event_type)
{
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
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    servaddr.sin_port = htons(SERVER_PORT);

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
    int s;

    /* open connection with the server */
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    s = DefaultScreen(display);

    /* create window */
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
    event_loop_x11();
    return 0;
}