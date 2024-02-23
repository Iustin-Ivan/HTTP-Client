#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <sys/queue.h>
#include "helpers.h"
#include "requests.h"
#include "buffer.h"
#include "parson.h"

// destul de clar ce sunt astea
#define SERVER_IP "34.254.242.81"
#define SERVER_PORT 8080
#define REGISTER "/api/v1/tema/auth/register"
#define LOGIN "/api/v1/tema/auth/login"
#define ACCESS "/api/v1/tema/library/access"
#define BOOKS "/api/v1/tema/library/books"
#define LOGOUT "/api/v1/tema/auth/logout"
#define FORMAT "application/json"

// la get_books cand parsez lista de jsoane folosesc lista asta
typedef struct list {
    char jsonobj[255];
    struct list* nextobj;
} list;

// variabile globale pe care le folosesc pentru a retine cookie-ul si token-ul si socket-ul server-ului
int sockfd;
char *cookie;
char *token;

void register_user (char *username, char *password) {
    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    // toate request-urile au formatul asta standard care apare in toate functiile
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    char *serialized_string = json_serialize_to_string_pretty(root_value);
    char *pack = compute_post_request(SERVER_IP, REGISTER, FORMAT, &serialized_string, 1, NULL, 0, NULL);
    send_to_server(sockfd, pack);
    char *response = receive_from_server(sockfd);
    if (strstr(response , "taken") != NULL) {
        printf("ERROR : User already exists! Choose another username!\n");
    } else {
        printf("SUCCES : User registered successfully!\n");
    }
}

void login_user (char* username, char* password) {
    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    char *serialized_string = json_serialize_to_string_pretty(root_value);
    char *pack = compute_post_request(SERVER_IP, LOGIN, FORMAT, &serialized_string, 1, NULL, 0, NULL);
    send_to_server(sockfd, pack);
    char *response = receive_from_server(sockfd);
    if (strstr(response , "Credentials") != NULL) {
        printf("ERROR : Wrong username or password!\n");
    } else if (strstr(response , "No account with this username") != NULL){
        printf("ERROR : No account with this username!\n");
    } else {
        printf("SUCCES : User logged in successfully!\n");
        cookie = strstr(response, "Set-Cookie");
        cookie = strtok(cookie, " ");
        cookie = strtok(NULL, ";");
    }
}

void access_library() {
    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    char *pack = compute_get_request(SERVER_IP, ACCESS, NULL, &cookie, 1, NULL);
    send_to_server(sockfd, pack);
    char *response = receive_from_server(sockfd);
    if (strstr(response , "You are not logged in!") != NULL) {
        printf("ERROR : You are not logged in!\n");
    } else {
        printf("SUCCES : You have access to the library!\n");
        token = strstr(response, "token");
        token = strtok(token, ":");
        token = strtok(NULL, "\"");
    }
}
// primesc de la server lista de json, o parsez si o afisez
// cate un obiect json pe rand
void parsejson(char *book) {
    char *bookcopy = (char*)malloc(strlen(book));
    memcpy(bookcopy, book, strlen(book));
    char *id = strtok(book, ":");
    id = strtok(NULL, ",");
    memcpy(book, bookcopy, strlen(book));
    char *title = strtok(bookcopy, ",");
    title = strtok(NULL, ":");
    title = strtok(NULL, "\"");
    printf("Book id: %s\n", id);
    printf("Book title: %s\n", title);
}

void list_books() {
    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    char *pack = compute_get_request(SERVER_IP, BOOKS, NULL, NULL, 0, token);
    send_to_server(sockfd, pack);
    char *response = receive_from_server(sockfd);
    if (strstr(response , "Error when decoding token") != NULL) {
        printf("ERROR : when decoding token. You are not authorized in the library! Use command enter_library to get a token!\n");
    } else {
        char *books = strstr(response, "[");
        books = books+1;
        books[strlen(books)-1] = '\0';
        // pt cand returneaza [] adica nu am nici o carte in biblioteca
        if (strlen(books) == 0) {
            printf("SUCCES : You have no books in the library! You can add one using add_book\n");
            return;
        }
        char *book = strtok(books, "}");
        list *head;
        head = (list*)malloc(sizeof(list));
        list *curr = head;
        // lista in care retin toate obiectele
        memcpy(head->jsonobj, book, strlen(book));
        while (book != NULL) {
            // fiecare obiect json este despartit de urmatorul prin }
            book = strtok(NULL, "}");
            if (book == NULL) {
                break;
            }
            book = book + 1;
            curr->nextobj = (list*)malloc(sizeof(list));
            memcpy(curr->nextobj->jsonobj, book, strlen(book));
            curr = curr->nextobj;
        }
        for (list *i = head; i != NULL; i = i->nextobj) {
            parsejson(i->jsonobj);
        }
    }
}

void show_book(char *bookID) {
    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    char url[255]={0};
    memcpy(url, "/api/v1/tema/library/books/", 27);
    memcpy(url + 27, bookID, strlen(bookID));
    char *pack = compute_get_request(SERVER_IP, url, NULL, NULL, 0, token);
    send_to_server(sockfd, pack);
    char *response = receive_from_server(sockfd);
    if (strstr(response , "Error when decoding token") != NULL) {
        printf("ERROR : when decoding token. You are not authorized in the library! Use enter_library to get a token!\n");
    } else {
        char *book = strstr(response, "{");
        char *valid = strstr(response, "title");
        if (valid) {
            // mult din tema asta a fost doar parsare ca sa fac afisari frumix
            char *title = strtok(book, ":");
            title = strtok(NULL, ":");
            char *author = strtok(NULL, ":");
            char *publisher = strtok(NULL, ":");
            char *genre = strtok(NULL, ":");
            char *page_count = strtok(NULL, ":");
            title = strtok(title, "\"");
            author = strtok(author, "\"");
            publisher = strtok(publisher, "\"");
            genre = strtok(genre, "\"");
            page_count[strlen(page_count)-1] = '\0';
            printf("Title=%s\n", title);
            printf("Author=%s\n", author);
            printf("Publisher=%s\n", publisher);
            printf("Genre=%s\n", genre);
            printf("Page count=%s\n", page_count);

        } else {
            printf("ERROR : No book with the ID %s!\n", bookID);
        }
    }
}

// regulile de respectat pt datele cartilor
int validate_book(char *title, char *author, char *genre, char *page_count, char *publisher) {
    if (strlen(title) == 0) {
        printf("ERROR : Title cannot be empty!\n");
        return 0;
    }
    if (title[0] == '\n' || title[0] == ' ') {
        printf("ERROR : Title cannot start with a space!\n");
        return 0;
    }
    if (strlen(author) == 0) {
        printf("ERROR : Author cannot be empty!\n");
        return 0;
    }
    if (author[0] == '\n' || author[0] == ' ') {
        printf("ERROR : Author cannot start with a space!\n");
        return 0;
    }
    if (strlen(genre) == 0) {
        printf("ERROR : Genre cannot be empty!\n");
        return 0;
    }
    if (genre[0] == '\n' || genre[0] == ' ') {
        printf("ERROR : Genre cannot start with a space!\n");
        return 0;
    }
    if (strlen(page_count) == 0) {
        printf("ERROR : Page count cannot be empty!\n");
        return 0;
    }
    for (int i = 0; i < strlen(page_count); i++) {
        if (page_count[i] < '0' || page_count[i] > '9') {
            printf("ERROR : Page count must be a number!\n");
            return 0;
        }
    }
    if (strlen(publisher) == 0) {
        printf("ERROR : Publisher cannot be empty!\n");
        return 0;
    }
    if (publisher[0] == '\n' || publisher[0] == ' ') {
        printf("ERROR : Publisher cannot start with a space!\n");
        return 0;
    }
    return 1;
}

void add_book (char *title, char *author, char *genre, int page_count, char *publisher) {
    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "title", title);
    json_object_set_string(root_object, "author", author);
    json_object_set_string(root_object, "genre", genre);
    json_object_set_number(root_object, "page_count", page_count);
    json_object_set_string(root_object, "publisher", publisher);
    char *serialized_string = json_serialize_to_string_pretty(root_value);
    char *pack = compute_post_request(SERVER_IP, BOOKS, FORMAT, &serialized_string, 1, NULL, 0, token);
    send_to_server(sockfd, pack);
    char *response = receive_from_server(sockfd);
    if (strstr(response , "Error when decoding token") != NULL) {
        printf("ERROR : when decoding token. You are not authorized in the library! Use enter_library to get a token!\n");
    } else {
        printf ("SUCCES : Book added!\n");
    }
}

void delete_book(char *bookID) {
    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    char url[255]={0};
    memcpy(url, "/api/v1/tema/library/books/", 27);
    memcpy(url + 27, bookID, strlen(bookID));
    char *pack = compute_delete_request(SERVER_IP, url, NULL, NULL, 0, token);
    send_to_server(sockfd, pack);
    char *response = receive_from_server(sockfd);
    if (strstr(response , "Error when decoding token") != NULL) {
        printf("ERROR : when decoding token. You are not authorized in the library! Use enter_library to get a token!\n");
    } else {
        char *valid = strstr(response, "No book was deleted");
        if (valid) {
            printf("ERROR : No book was deleted because there is no book with the ID %s!\n", bookID);
        } else {
            printf("SUCCES : Book deleted with ID %s!\n", bookID);
        }
    }
}

void logout() {
    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    char *pack = compute_get_request(SERVER_IP, LOGOUT, NULL, &cookie, 1, NULL);
    send_to_server(sockfd, pack);
    char *response = receive_from_server(sockfd);
    if (strstr(response , "You are not logged in") != NULL) {
        printf("ERROR : You are not logged in! login to get a cookie!\n");
    } else {
        printf("SUCCES : Logged out!\n");
    }
    // la logout se sterg cookie-ul si token-ul
    cookie = "nocookie";
    token = "notoken";
}

void parse_command(char* command) {
    if (strncmp(command, "register", 8)==0 && strlen(command) == 8) {
        char username[255]={0};
        char password[255]={0};
        printf("username = ");
        fgets(username, 255, stdin);
        username[strlen(username)-1] = '\0';
        printf("password = ");
        fgets(password, 255, stdin);
        password[strlen(password)-1] = '\0';
        register_user(username, password);
    } else if (strncmp(command, "login", 5)==0) {
        char username[255];
        char password[255];
        printf("username = ");
        fgets(username, 255, stdin);
        username[strlen(username)-1] = '\0';
        printf("password = ");
        fgets(password, 255, stdin);
        password[strlen(password)-1] = '\0';
        login_user(username, password);
    } else if (strncmp(command, "enter_library", 13)==0 && strlen(command) == 13) {
        access_library();
    } else if (strncmp(command, "get_books", 9)==0 && strlen(command) == 9) {
        list_books();
    } else if (strncmp(command, "get_book", 8)==0 && strlen(command) == 8) {
        char bookID[255];
        printf("id = ");
        fgets(bookID, 255, stdin);
        bookID[strlen(bookID)-1] = '\0';
        for (int i = 0; i < strlen(bookID); i++) {
            if (bookID[i] < '0' || bookID[i] > '9') {
                printf("ERROR : Invalid ID! Must contain numbers only!\n");
                return;
            }
        }
        show_book(bookID);
    } else if (strncmp(command, "add_book", 8)==0 && strlen(command) == 8) {
        char title[255];
        char author[255];
        char genre[255];
        char page_count[255];
        char publisher[255];
        printf("title = ");
        fgets(title, 255, stdin);
        title[strlen(title)-1] = '\0';
        printf("author = ");
        fgets(author, 255, stdin);
        author[strlen(author)-1] = '\0';
        printf("genre = ");
        fgets(genre, 255, stdin);
        genre[strlen(genre)-1] = '\0';
        printf("page_count = ");
        fgets(page_count, 255, stdin);
        page_count[strlen(page_count)-1] = '\0';
        printf("publisher = ");
        fgets(publisher, 255, stdin);
        publisher[strlen(publisher)-1] = '\0';
        int ok = validate_book(title, author, genre, page_count, publisher);
        if (ok == 0) {
            return;
        }
        int page_count_int = atoi(page_count);
        add_book(title, author, genre, page_count_int, publisher);
    } else if (strncmp(command, "delete_book", 11)==0 && strlen(command) == 11) {
        char bookID[255];
        printf("id = ");
        fgets(bookID, 255, stdin);
        bookID[strlen(bookID)-1] = '\0';
        for (int i = 0; i < strlen(bookID); i++) {
            if (bookID[i] < '0' || bookID[i] > '9') {
                printf("ERROR : Invalid ID! Must contain numbers only!\n");
                return;
            }
        }
        delete_book(bookID);
    } else if (strncmp(command, "logout", 6)==0 && strlen(command) == 6) {
          logout();
    } else {
        printf("ERROR : Invalid command! Refer to function parse_command from client.c for valid ones\n");
    }
}

int main(int argc, char *argv[]) {
    setvbuf (stdout, NULL, _IONBF, BUFSIZ);
    cookie = "nocookie";
    token = "notoken";

    while (1) {
        char line[255] = {0};
        fgets(line, 255, stdin);
        line[strlen(line) - 1] = '\0';
        if (strcmp(line, "exit") == 0 && strlen(line) == 4) {
            close(sockfd);
            exit(0);
        } else {
            parse_command(line);
        }
    }

    return 0;
}
