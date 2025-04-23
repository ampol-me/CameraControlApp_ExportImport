#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

// โครงสร้างสำหรับเก็บ response
typedef struct {
    char *data;
    size_t size;
} ResponseData;

// ฟังก์ชัน callback สำหรับ CURL
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);

// ฟังก์ชันสำหรับส่ง HTTP GET request
char* http_get(const char *url, const char *user, const char *pass);

#endif