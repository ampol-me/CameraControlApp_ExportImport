#include "http_client.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ฟังก์ชัน callback สำหรับ CURL
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    ResponseData *mem = (ResponseData *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        printf("ไม่สามารถจัดสรรหน่วยความจำได้\n");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

// ฟังก์ชันสำหรับส่ง HTTP GET request
char* http_get(const char *url, const char *user, const char *pass) {
    CURL *curl;
    CURLcode res;
    ResponseData chunk = {0};

    curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }

    // ตั้งค่า CURL options
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    // ตั้งค่า authentication ถ้ามี
    if (user && pass) {
        char auth[256];
        snprintf(auth, sizeof(auth), "%s:%s", user, pass);
        curl_easy_setopt(curl, CURLOPT_USERPWD, auth);
    }

    // ส่ง request
    res = curl_easy_perform(curl);

    // ตรวจสอบผลลัพธ์
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.data);
        chunk.data = NULL;
    }

    curl_easy_cleanup(curl);
    return chunk.data;
}