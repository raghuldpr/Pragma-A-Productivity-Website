#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void cgi_json_header(void) {
    printf("Content-Type: application/json\r\n");
    printf("Access-Control-Allow-Origin: *\r\n");
    printf("Access-Control-Allow-Methods: GET, POST\r\n");
    printf("Access-Control-Allow-Headers: Content-Type\r\n");
    printf("\r\n");
}

void json_error(const char *msg) {
    cgi_json_header();
    printf("{\"success\":false,\"error\":\"%s\"}", msg);
}

void json_ok(const char *payload) {
    cgi_json_header();
    printf("{\"success\":true,%s}", payload);
}

void url_decode(char *dst, const char *src, size_t max) {
    size_t i = 0;
    while (*src && i < max - 1) {
        if (*src == '%' && src[1] && src[2]) {
            char hex[3] = { src[1], src[2], 0 };
            dst[i++] = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            dst[i++] = ' '; src++;
        } else {
            dst[i++] = *src++;
        }
    }
    dst[i] = '\0';
}

void get_field(const char *body, const char *key, char *out, size_t max) {
    out[0] = '\0';
    if (!body) return;
    char search[128];
    snprintf(search, sizeof(search), "%s=", key);
    const char *p = strstr(body, search);
    if (!p) return;
    p += strlen(search);
    char raw[2048] = {0};
    size_t i = 0;
    while (*p && *p != '&' && i < sizeof(raw)-1)
        raw[i++] = *p++;
    raw[i] = '\0';
    url_decode(out, raw, max);
}

char *read_post(void) {
    const char *len_str = getenv("CONTENT_LENGTH");
    if (!len_str) return NULL;
    int len = atoi(len_str);
    if (len <= 0 || len > 65536) return NULL;
    char *buf = (char*)malloc(len + 1);
    if (!buf) return NULL;
    fread(buf, 1, len, stdin);
    buf[len] = '\0';
    return buf;
}

void sanitize(char *s) {
    for (; *s; s++)
        if (*s=='\'' || *s=='"' || *s=='\\' || *s==';')
            *s = '_';
}

void gen_token(char *out, size_t len) {
    const char *h = "0123456789abcdef";
    srand((unsigned)time(NULL) ^ (unsigned)(size_t)out);
    for (size_t i = 0; i < len-1; i++)
        out[i] = h[rand() % 16];
    out[len-1] = '\0';
}

void hash_pass(const char *pass, char *out, size_t outlen) {
    unsigned char key = 0x5A;
    size_t plen = strlen(pass);
    char *p = out;
    size_t remaining = outlen;
    for (size_t i = 0; i < plen && remaining > 2; i++, p+=2, remaining-=2)
        snprintf(p, remaining, "%02x", (unsigned char)(pass[i] ^ (key + i)));
    *p = '\0';
}

int verify_pass(const char *pass, const char *stored) {
    char computed[512];
    hash_pass(pass, computed, sizeof(computed));
    return strcmp(computed, stored) == 0;
}

#endif