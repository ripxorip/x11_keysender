#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define CONFIG_FILE "client.txt"

char dictation_ip[INET_ADDRSTRLEN] = "";

void press_key(KeySym keysym) {
    // Fake a F10 key press for X11
    Display *display = XOpenDisplay(NULL);
    XTestFakeKeyEvent(display, XKeysymToKeycode(display, keysym), True, 0);
    XTestFakeKeyEvent(display, XKeysymToKeycode(display, keysym), False, 0);
    XCloseDisplay(display);
}

void press_stop_key() {
    press_key(XK_F10);
}

void press_dictation_key() {
    press_key(XK_F11);
}

void press_command_key() {
    press_key(XK_F12);
}

void *listen_udp(void *arg) {
    (void)arg;
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    // Create socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(5000);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        uint32_t len, n;
        char buffer[1024];
        len = sizeof(cliaddr); //len is value/resuslt

        n = recvfrom(sockfd, (char *)buffer, 1024, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';

        // Check if the message is "start_dictation@ip_addr"
        if (strncmp(buffer, "start_dictation@", 16) == 0) {
            // Copy the IP address to the global variable
            strncpy(dictation_ip, buffer + 16, INET_ADDRSTRLEN);
            press_dictation_key();
        }
        else if (strncmp(buffer, "start_command@", 14) == 0) {
            // Copy the IP address to the global variable
            strncpy(dictation_ip, buffer + 14, INET_ADDRSTRLEN);
            press_command_key();
        }
        else if (strncmp(buffer, "stop", 4) == 0) {
            dictation_ip[0] = '\0';
            press_stop_key();
        }
    }
    return NULL;
}

void send_keyevent_udp(unsigned int keycode, int event_type)
{
    if (strlen(dictation_ip) == 0)
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
    servaddr.sin_addr.s_addr = inet_addr(dictation_ip);
    servaddr.sin_port = htons(5000);

    /* send key event to server */
    char buf[100];
    snprintf(buf, sizeof(buf), "%x,%d", keycode, event_type);
    sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&dictation_ip, sizeof(servaddr));

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
    pthread_t thread_id;
    // Create a new thread
    if (pthread_create(&thread_id, NULL, listen_udp, NULL) != 0) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    /* start event loop */
    event_loop_x11();

    return 0;
}