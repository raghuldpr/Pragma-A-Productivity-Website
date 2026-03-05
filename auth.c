#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include "config.h"
#include "utils.h"

MYSQL *db_connect(void) {
    MYSQL *conn = mysql_init(NULL);
    if (!conn) return NULL;
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS,
                            DB_NAME, DB_PORT, NULL, 0)) {
        mysql_close(conn); return NULL;
    }
    mysql_set_character_set(conn, "utf8mb4");
    return conn;
}

void do_register(MYSQL *conn, const char *body) {
    char name[128], gmail[256], pass[256], hash[512];
    get_field(body, "full_name", name,  sizeof(name));
    get_field(body, "gmail",     gmail, sizeof(gmail));
    get_field(body, "password",  pass,  sizeof(pass));

    if (!strlen(name))  { json_error("Full name is required");  return; }
    if (!strlen(gmail)) { json_error("Gmail is required");      return; }
    if (!strlen(pass))  { json_error("Password is required");   return; }

    if (!strstr(gmail, "@gmail.com")) {
        json_error("Only @gmail.com addresses are allowed");
        return;
    }
    if (strlen(pass) < 8) {
        json_error("Password must be at least 8 characters");
        return;
    }

    sanitize(name); sanitize(gmail);

    char q[512];
    snprintf(q, sizeof(q),
             "SELECT id FROM users WHERE gmail='%s' LIMIT 1", gmail);
    mysql_query(conn, q);
    MYSQL_RES *res = mysql_store_result(conn);
    int exists = (mysql_num_rows(res) > 0);
    mysql_free_result(res);
    if (exists) { json_error("Account already exists"); return; }

    hash_pass(pass, hash, sizeof(hash));
    snprintf(q, sizeof(q),
             "INSERT INTO users (full_name, gmail, password_hash) "
             "VALUES ('%.127s','%.254s','%.511s')",
             name, gmail, hash);

    if (mysql_query(conn, q) != 0) {
        json_error("Registration failed"); return;
    }
    json_ok("\"message\":\"Account created! Please sign in.\"");
}

void do_login(MYSQL *conn, const char *body) {
    char gmail[256], pass[256];
    get_field(body, "gmail",    gmail, sizeof(gmail));
    get_field(body, "password", pass,  sizeof(pass));

    if (!strlen(gmail) || !strlen(pass)) {
        json_error("Gmail and password are required"); return;
    }
    sanitize(gmail);

    char q[512];
    snprintf(q, sizeof(q),
             "SELECT id, password_hash, full_name FROM users "
             "WHERE gmail='%.254s' AND is_active=1 LIMIT 1", gmail);

    if (mysql_query(conn, q) != 0) { json_error("Database error"); return; }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW  row = mysql_fetch_row(res);

    if (!row) {
        mysql_free_result(res);
        json_error("Invalid Gmail or password"); return;
    }

    int  uid = atoi(row[0]);
    char stored[512], full_name[128];
    strncpy(stored,    row[1],         sizeof(stored)-1);
    strncpy(full_name, row[2]?row[2]:"", sizeof(full_name)-1);
    mysql_free_result(res);

    if (!verify_pass(pass, stored)) {
        json_error("Invalid Gmail or password"); return;
    }

    char token[TOKEN_LEN + 1];
    gen_token(token, sizeof(token));

    time_t expiry = time(NULL) + SESSION_HOURS * 3600;
    struct tm *te = gmtime(&expiry);
    char expiry_str[32];
    strftime(expiry_str, sizeof(expiry_str), "%Y-%m-%d %H:%M:%S", te);

    snprintf(q, sizeof(q),
             "INSERT INTO sessions (user_id, token, expires_at) "
             "VALUES (%d, '%s', '%s')",
             uid, token, expiry_str);
    mysql_query(conn, q);

    snprintf(q, sizeof(q),
             "UPDATE users SET last_login=NOW() WHERE id=%d", uid);
    mysql_query(conn, q);

    cgi_json_header();
    printf("{\"success\":true,\"token\":\"%s\","
           "\"user_id\":%d,\"full_name\":\"%s\",\"gmail\":\"%s\"}",
           token, uid, full_name, gmail);
}

void do_logout(MYSQL *conn, const char *body) {
    char token[TOKEN_LEN + 8];
    get_field(body, "token", token, sizeof(token));
    sanitize(token);
    char q[256];
    snprintf(q, sizeof(q),
             "DELETE FROM sessions WHERE token='%s'", token);
    mysql_query(conn, q);
    json_ok("\"message\":\"Logged out\"");
}

int main(void) {
    const char *method = getenv("REQUEST_METHOD");
    if (method && strcmp(method,"OPTIONS")==0) { cgi_json_header(); return 0; }

    const char *qs = getenv("QUERY_STRING");
    char action[32] = {0};
    if (qs) get_field(qs, "action", action, sizeof(action));

    char *body = NULL;
    if (method && strcmp(method,"POST")==0) body = read_post();

    MYSQL *conn = db_connect();
    if (!conn) { json_error("Cannot connect to database"); free(body); return 1; }

    if      (strcmp(action,"register")==0) do_register(conn, body?body:"");
    else if (strcmp(action,"login")   ==0) do_login   (conn, body?body:"");
    else if (strcmp(action,"logout")  ==0) do_logout  (conn, body?body:"");
    else json_error("Unknown action");

    mysql_close(conn);
    free(body);
    return 0;
}