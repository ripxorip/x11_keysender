#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

// In the future, one shall be able to connect to this computer and register a callback
// instead

#define SERVER_URL_PRESS "http://voiceboxclient:5000/key_press"
#define SERVER_URL_RELEASE "http://voiceboxclient:5000/key_release"

static size_t callback_http(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    /* ignore response */
    return size * nmemb;
}

void send_keypress_http(unsigned int keycode, const char *url)
{
    /* initialize libcurl */
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    if (curl == NULL)
    {
        fprintf(stderr, "Cannot create libcurl handle\n");
        exit(1);
    }

    /* set HTTP headers */
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

    /* send key event to server */
    char buf[100];
    snprintf(buf, sizeof(buf), "key=%x", keycode);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buf);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_http);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        fprintf(stderr, "Cannot send HTTP request: %s\n", curl_easy_strerror(res));
    }

    /* cleanup */
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
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
            send_keypress_http(event.xkey.keycode, SERVER_URL_PRESS);
        }
        else if (event.type == KeyRelease)
        {
            printf("KeyRelease: %x\n", event.xkey.keycode);

            /* send key release event to server */
            send_keypress_http(event.xkey.keycode, SERVER_URL_RELEASE);
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