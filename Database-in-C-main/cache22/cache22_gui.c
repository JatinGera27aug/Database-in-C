#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")

#define WM_SERVER_MSG (WM_USER + 1)

static HWND hOutput, hHost, hPort, hKey, hValue, hTtl, hTopN, hConnect;
static SOCKET g_sock = INVALID_SOCKET;
static HANDLE g_recvThread = NULL;
static volatile LONG g_connected = 0;
static HWND g_hWndMain = NULL;

static void append_output(const char *s) {
    if (!hOutput) return;
    int len = (int)SendMessageA(hOutput, WM_GETTEXTLENGTH, 0, 0);
    SendMessageA(hOutput, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageA(hOutput, EM_REPLACESEL, FALSE, (LPARAM)s);
}

static DWORD WINAPI recv_thread(LPVOID param) {
    (void)param;
    char buf[2048];
    while (g_connected) {
        int n = recv(g_sock, buf, sizeof(buf)-1, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        // copy and post to main thread
        char *copy = _strdup(buf);
        if (copy) {
            PostMessageA(g_hWndMain, WM_SERVER_MSG, 0, (LPARAM)copy);
        }
    }
    // notify main thread that connection closed
    PostMessageA(g_hWndMain, WM_SERVER_MSG, 0, (LPARAM)_strdup("** disconnected **\r\n"));
    InterlockedExchange(&g_connected, 0);
    return 0;
}

static int try_connect(const char *host, const char *port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        MessageBoxA(NULL, "WSAStartup failed", "Error", MB_ICONERROR);
        return -1;
    }

    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        MessageBoxA(NULL, "getaddrinfo failed", "Error", MB_ICONERROR);
        WSACleanup();
        return -1;
    }

    g_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (g_sock == INVALID_SOCKET) {
        freeaddrinfo(res);
        WSACleanup();
        MessageBoxA(NULL, "socket() failed", "Error", MB_ICONERROR);
        return -1;
    }

    if (connect(g_sock, res->ai_addr, (int)res->ai_addrlen) != 0) {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
        freeaddrinfo(res);
        WSACleanup();
        MessageBoxA(NULL, "connect() failed", "Error", MB_ICONERROR);
        return -1;
    }

    freeaddrinfo(res);
    return 0;
}

static void do_disconnect() {
    if (g_recvThread) {
        // signal thread by closing socket
        shutdown(g_sock, SD_BOTH);
        WaitForSingleObject(g_recvThread, 2000);
        CloseHandle(g_recvThread);
        g_recvThread = NULL;
    }
    if (g_sock != INVALID_SOCKET) {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
    WSACleanup();
    InterlockedExchange(&g_connected, 0);
    SetWindowTextA(hConnect, "Connect");
}

static void send_line(const char *line) {
    if (!g_connected) {
        MessageBoxA(NULL, "Not connected", "Info", MB_OK | MB_ICONINFORMATION);
        return;
    }
    // ensure CRLF
    char out[2048];
    snprintf(out, sizeof(out), "%s\r\n", line);
    send(g_sock, out, (int)strlen(out), 0);
    // echo to output
    char echo[2050];
    snprintf(echo, sizeof(echo), "> %s\r\n", line);
    append_output(echo);
}

static void on_connect_toggle(void) {
    if (!g_connected) {
        char host[128], port[32];
        GetWindowTextA(hHost, host, sizeof(host));
        GetWindowTextA(hPort, port, sizeof(port));
        if (host[0] == '\0') strcpy(host, "127.0.0.1");
        if (port[0] == '\0') strcpy(port, "8080");

        if (try_connect(host, port) == 0) {
            InterlockedExchange(&g_connected, 1);
            SetWindowTextA(hConnect, "Disconnect");
            append_output("** connected **\r\n");
            g_recvThread = CreateThread(NULL, 0, recv_thread, NULL, 0, NULL);
            if (!g_recvThread) {
                MessageBoxA(NULL, "Failed to create recv thread", "Error", MB_ICONERROR);
                do_disconnect();
            }
        }
    } else {
        do_disconnect();
        append_output("** disconnected by user **\r\n");
    }
}

static void on_add() {
    char key[128], val[512], ttl[32];
    GetWindowTextA(hKey, key, sizeof(key));
    GetWindowTextA(hValue, val, sizeof(val));
    GetWindowTextA(hTtl, ttl, sizeof(ttl));
    if (key[0] == '\0' || val[0] == '\0') {
        MessageBoxA(NULL, "Key and value required", "Info", MB_OK | MB_ICONINFORMATION);
        return;
    }
    char line[1024];
    if (ttl[0]) snprintf(line, sizeof(line), "add %s %s -ttl %s", key, val, ttl);
    else snprintf(line, sizeof(line), "add %s %s", key, val);
    send_line(line);
}

static void on_serve() {
    char key[128];
    GetWindowTextA(hKey, key, sizeof(key));
    if (key[0] == '\0') { MessageBoxA(NULL, "Key required", "Info", MB_OK|MB_ICONINFORMATION); return; }
    char line[256];
    snprintf(line, sizeof(line), "serve %s", key);
    send_line(line);
}

static void on_del() {
    char key[128];
    GetWindowTextA(hKey, key, sizeof(key));
    if (key[0] == '\0') { MessageBoxA(NULL, "Key required", "Info", MB_OK|MB_ICONINFORMATION); return; }
    char line[256];
    snprintf(line, sizeof(line), "del %s", key);
    send_line(line);
}

static void on_flush() {
    send_line("flush");
}

static void on_top() {
    char n[32];
    GetWindowTextA(hTopN, n, sizeof(n));
    if (n[0] == '\0') send_line("top");
    else {
        char line[64];
        snprintf(line, sizeof(line), "top %s", n);
        send_line(line);
    }
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        {
            g_hWndMain = hWnd;
            CreateWindowA("STATIC", "Host:", WS_CHILD|WS_VISIBLE, 8, 8, 40, 20, hWnd, NULL, NULL, NULL);
            hHost = CreateWindowA("EDIT", "127.0.0.1", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT, 50, 6, 120, 22, hWnd, NULL, NULL, NULL);
            CreateWindowA("STATIC", "Port:", WS_CHILD|WS_VISIBLE, 180, 8, 36, 20, hWnd, NULL, NULL, NULL);
            hPort = CreateWindowA("EDIT", "8080", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT, 214, 6, 60, 22, hWnd, NULL, NULL, NULL);

            hConnect = CreateWindowA("BUTTON", "Connect", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 290, 6, 80, 24, hWnd, (HMENU)1, NULL, NULL);

            CreateWindowA("STATIC", "Key:", WS_CHILD|WS_VISIBLE, 8, 40, 36, 20, hWnd, NULL, NULL, NULL);
            hKey = CreateWindowA("EDIT", "", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT, 50, 38, 180, 22, hWnd, NULL, NULL, NULL);

            CreateWindowA("STATIC", "Value:", WS_CHILD|WS_VISIBLE, 240, 40, 40, 20, hWnd, NULL, NULL, NULL);
            hValue = CreateWindowA("EDIT", "", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT, 280, 38, 220, 22, hWnd, NULL, NULL, NULL);

            CreateWindowA("STATIC", "TTL (s):", WS_CHILD|WS_VISIBLE, 8, 72, 60, 20, hWnd, NULL, NULL, NULL);
            hTtl = CreateWindowA("EDIT", "", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT, 70, 70, 80, 22, hWnd, NULL, NULL, NULL);

            CreateWindowA("STATIC", "Top N:", WS_CHILD|WS_VISIBLE, 160, 72, 56, 20, hWnd, NULL, NULL, NULL);
            hTopN = CreateWindowA("EDIT", "5", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT, 220, 70, 60, 22, hWnd, NULL, NULL, NULL);

            CreateWindowA("BUTTON", "Add", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 300, 68, 60, 24, hWnd, (HMENU)2, NULL, NULL);
            CreateWindowA("BUTTON", "Serve", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 370, 68, 60, 24, hWnd, (HMENU)3, NULL, NULL);
            CreateWindowA("BUTTON", "Del", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 440, 68, 60, 24, hWnd, (HMENU)4, NULL, NULL);
            CreateWindowA("BUTTON", "Flush", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 510, 68, 60, 24, hWnd, (HMENU)5, NULL, NULL);
            CreateWindowA("BUTTON", "Top", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 580, 68, 60, 24, hWnd, (HMENU)6, NULL, NULL);

            hOutput = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,
                                      8, 100, 640, 360, hWnd, NULL, NULL, NULL);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1: on_connect_toggle(); break;
        case 2: on_add(); break;
        case 3: on_serve(); break;
        case 4: on_del(); break;
        case 5: on_flush(); break;
        case 6: on_top(); break;
        }
        break;
    case WM_SERVER_MSG:
        {
            char *ptr = (char*)lParam;
            if (ptr) {
                append_output(ptr);
                free(ptr);
            }
        }
        break;
    case WM_CLOSE:
        do_disconnect();
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "Cache22GUI";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, wc.lpszClassName, "cache22 GUI client", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                               CW_USEDEFAULT, CW_USEDEFAULT, 670, 520, NULL, NULL, hInstance, NULL);
    if (!hwnd) return 0;
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}