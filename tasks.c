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

int validate_token(MYSQL *conn, const char *token) {
    if (!token || strlen(token) < 8) return -1;
    char safe[TOKEN_LEN + 8];
    strncpy(safe, token, sizeof(safe)-1);
    safe[sizeof(safe)-1] = '\0';
    sanitize(safe);
    char q[512];
    snprintf(q, sizeof(q),
             "SELECT user_id FROM sessions "
             "WHERE token='%s' AND expires_at > NOW() LIMIT 1", safe);
    if (mysql_query(conn, q) != 0) return -1;
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW  row = mysql_fetch_row(res);
    int uid = row ? atoi(row[0]) : -1;
    mysql_free_result(res);
    return uid;
}

void do_list(MYSQL *conn, const char *qs) {
    char token[TOKEN_LEN+8], filter[32];
    get_field(qs, "token",  token,  sizeof(token));
    get_field(qs, "filter", filter, sizeof(filter));

    int uid = validate_token(conn, token);
    if (uid < 0) { json_error("Unauthorized"); return; }

    char where[64] = "";
    if      (strcmp(filter,"pending")==0) strcpy(where," AND status='pending'");
    else if (strcmp(filter,"done")   ==0) strcpy(where," AND status='done'");
    else if (strcmp(filter,"high")   ==0) strcpy(where," AND priority=3 AND status='pending'");

    char q[512];
    snprintf(q, sizeof(q),
             "SELECT id,title,category,priority,status,"
             "DATE_FORMAT(due_date,'%%Y-%%m-%%d'),"
             "DATE_FORMAT(created_at,'%%Y-%%m-%%d') "
             "FROM tasks WHERE user_id=%d%s "
             "ORDER BY priority DESC, created_at DESC",
             uid, where);

    if (mysql_query(conn,q)!=0) { json_error("Query failed"); return; }
    MYSQL_RES *res = mysql_store_result(conn);

    cgi_json_header();
    printf("{\"success\":true,\"count\":%llu,\"tasks\":[",
           (unsigned long long)mysql_num_rows(res));

    MYSQL_ROW row; int first=1;
    while ((row = mysql_fetch_row(res))) {
        if (!first) printf(",");
        first=0;
        printf("{\"id\":%s,\"title\":\"%s\",\"category\":\"%s\","
               "\"priority\":%s,\"status\":\"%s\","
               "\"due_date\":\"%s\",\"created_at\":\"%s\"}",
               row[0], row[1]?row[1]:"",
               row[2]?row[2]:"other",
               row[3]?row[3]:"2",
               row[4]?row[4]:"pending",
               row[5]?row[5]:"",
               row[6]?row[6]:"");
    }
    printf("]}");
    mysql_free_result(res);
}

void do_add(MYSQL *conn, const char *body) {
    char token[TOKEN_LEN+8], title[256], cat[32], pri[4], due[16];
    get_field(body,"token",    token, sizeof(token));
    get_field(body,"title",    title, sizeof(title));
    get_field(body,"category", cat,   sizeof(cat));
    get_field(body,"priority", pri,   sizeof(pri));
    get_field(body,"due_date", due,   sizeof(due));

    int uid = validate_token(conn, token);
    if (uid < 0) { json_error("Unauthorized"); return; }
    if (!strlen(title)) { json_error("Title is required"); return; }

    int p = atoi(pri);
    if (p<1||p>3) p=2;
    sanitize(title); sanitize(cat);

    const char *cats[]={"work","personal","health","finance","learning","other"};
    int valid=0;
    for (int i=0;i<6;i++) if(strcmp(cat,cats[i])==0){valid=1;break;}
    if (!valid) strcpy(cat,"other");

    char due_val[32]="NULL";
    if (strlen(due)==10) snprintf(due_val,sizeof(due_val),"'%s'",due);

    char q[1024];
    snprintf(q,sizeof(q),
             "INSERT INTO tasks (user_id,title,category,priority,due_date) "
             "VALUES (%d,'%.254s','%s',%d,%s)",
             uid,title,cat,p,due_val);

    if (mysql_query(conn,q)!=0) { json_error("Failed to add task"); return; }
    cgi_json_header();
    printf("{\"success\":true,\"task_id\":%llu}",
           (unsigned long long)mysql_insert_id(conn));
}

void do_update(MYSQL *conn, const char *body) {
    char token[TOKEN_LEN+8], tid[16], status[16];
    get_field(body,"token",   token,  sizeof(token));
    get_field(body,"task_id", tid,    sizeof(tid));
    get_field(body,"status",  status, sizeof(status));

    int uid = validate_token(conn, token);
    if (uid < 0) { json_error("Unauthorized"); return; }

    if (strcmp(status,"pending")!=0 && strcmp(status,"done")!=0) {
        json_error("Invalid status"); return;
    }

    char q[256];
    snprintf(q,sizeof(q),
             "UPDATE tasks SET status='%s' WHERE id=%d AND user_id=%d",
             status, atoi(tid), uid);

    if (mysql_query(conn,q)!=0) { json_error("Update failed"); return; }
    json_ok("\"message\":\"Task updated\"");
}

void do_delete(MYSQL *conn, const char *body) {
    char token[TOKEN_LEN+8], tid[16];
    get_field(body,"token",   token, sizeof(token));
    get_field(body,"task_id", tid,   sizeof(tid));

    int uid = validate_token(conn, token);
    if (uid < 0) { json_error("Unauthorized"); return; }

    char q[256];
    snprintf(q,sizeof(q),
             "DELETE FROM tasks WHERE id=%d AND user_id=%d",
             atoi(tid), uid);

    if (mysql_query(conn,q)!=0) { json_error("Delete failed"); return; }
    json_ok("\"message\":\"Task deleted\"");
}

void do_stats(MYSQL *conn, const char *qs) {
    char token[TOKEN_LEN+8];
    get_field(qs,"token",token,sizeof(token));
    int uid = validate_token(conn,token);
    if (uid<0) { json_error("Unauthorized"); return; }

    char q[512];
    snprintf(q,sizeof(q),
             "SELECT COUNT(*),SUM(status='done'),SUM(status='pending'),"
             "SUM(priority=3 AND status='pending'),"
             "SUM(due_date=CURDATE() AND status='pending') "
             "FROM tasks WHERE user_id=%d", uid);

    if (mysql_query(conn,q)!=0) { json_error("Stats failed"); return; }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW  row = mysql_fetch_row(res);

    cgi_json_header();
    printf("{\"success\":true,\"stats\":{"
           "\"total\":%s,\"done\":%s,\"pending\":%s,"
           "\"urgent\":%s,\"due_today\":%s}}",
           row[0]?row[0]:"0", row[1]?row[1]:"0",
           row[2]?row[2]:"0", row[3]?row[3]:"0",
           row[4]?row[4]:"0");
    mysql_free_result(res);
}

int main(void) {
    const char *method = getenv("REQUEST_METHOD");
    if (method && strcmp(method,"OPTIONS")==0) { cgi_json_header(); return 0; }

    const char *qs = getenv("QUERY_STRING");
    char action[32]={0};
    if (qs) get_field(qs,"action",action,sizeof(action));

    char *body=NULL;
    if (method && strcmp(method,"POST")==0) body=read_post();

    MYSQL *conn=db_connect();
    if (!conn) { json_error("Cannot connect to database"); free(body); return 1; }

    if      (strcmp(action,"list")  ==0) do_list  (conn, qs?qs:"");
    else if (strcmp(action,"add")   ==0) do_add   (conn, body?body:"");
    else if (strcmp(action,"update")==0) do_update(conn, body?body:"");
    else if (strcmp(action,"delete")==0) do_delete(conn, body?body:"");
    else if (strcmp(action,"stats") ==0) do_stats (conn, qs?qs:"");
    else json_error("Unknown action");

    mysql_close(conn);
    free(body);
    return 0;
}