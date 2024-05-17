#include <iostream>
#include <WS2tcpip.h>
#include <winsock2.h>
#include <mysql.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libmysql.lib")

SOCKET Connection;
MYSQL* conn; // Глобальная переменная для подключения к MySQL

char username[256]; // Глобальная переменная для хранения имени пользователя

void ClientHandler() {
    char msg[256];
    char log[256];
    while (true) {
        int bytesReceived = recv(Connection, log, sizeof(log), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Ошибка отправки сообщения на сервер или соединение закрыто." << std::endl;
            break;
        }
        log[bytesReceived] = '\0';

        bytesReceived = recv(Connection, msg, sizeof(msg), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Ошибка отправки сообщения на сервер или соединение закрыто." << std::endl;
            break;
        }
        msg[bytesReceived] = '\0';

        std::cout << msg << ":" << log << std::endl;
    }
}

void regist() {
    char log1[256];
    char pass[256];

    std::cout << "Регистрация|Введите логин:" << std::endl;
    std::cin >> log1;
    std::cout << "Регистрация|Введите пароль:" << std::endl;
    std::cin >> pass;

    std::string insert_query = "INSERT INTO users (username, password) VALUES ('" + std::string(log1) + "', '" + std::string(pass) + "')";
    if (mysql_query(conn, insert_query.c_str())) {
        std::cerr << "Ошибка: " << mysql_error(conn) << std::endl;
    }
    else {
        std::cout << "Регистрация успешна! Пожалуйста, войдите в систему." << std::endl;
    }
}

void login() {
    char log1[256];
    char pass[256];

    std::cout << "Авторизация|Введите логин:" << std::endl;
    std::cin >> log1;
    std::cout << "Авторизация|Введите пароль:" << std::endl;
    std::cin >> pass;

    std::string select_query = "SELECT * FROM users WHERE username='" + std::string(log1) + "' AND password='" + std::string(pass) + "'";
    if (mysql_query(conn, select_query.c_str())) {
        std::cerr << "Ошибка: " << mysql_error(conn) << std::endl;
    }
    else {
        MYSQL_RES* result = mysql_store_result(conn);
        if (result) {
            if (mysql_num_rows(result) == 1) {
                std::cout << "С возвращением! " << log1 << std::endl;
                strcpy_s(username, log1);  // Сохраняем имя пользователя
            }
            else {
                std::cout << "Логин или пароль введены неверно!" << std::endl;
            }
            mysql_free_result(result);
        }
        else {
            std::cerr << "Ошибка: " << mysql_error(conn) << std::endl;
        }
    }
}
void sendMessageToDatabase(const char* sender, const char* message) {
    char query[512];
    sprintf_s(query, "INSERT INTO messages (sender, message) VALUES ('%s', '%s')", sender, message);

    if (mysql_query(conn, query)) {
        std::cerr << "Ошибка отправки сообщения в базу данных: " << mysql_error(conn) << std::endl;
    }
    else {
        std::cout << "Сообщение отправлено в базу данных успешно!" << std::endl;
    }
}
void chat() {
    char msg[512];
    std::cin.ignore(INT_MAX, '\n');
    while (true) {
        std::cout << "Введите сообщение:" << std::endl;
        std::cin.getline(msg, sizeof(msg));
        sendMessageToDatabase(username, msg); // Передаем имя отправителя в функцию
    }
}

int main() {
    setlocale(LC_ALL, "");
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0) {
        std::cout << "Ошибка" << std::endl;
        exit(1);
    }

    conn = mysql_init(NULL);
    if (!conn) {
        std::cerr << "Ошибка: " << mysql_error(conn) << std::endl;
        return 1;
    }

    if (!mysql_real_connect(conn, "localhost", "Niko", "", "chat_db", 3306, NULL, 0)) {
        std::cerr << "Ошибка: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return 1;
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr)); // Обнуляем структуру
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(7777);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if (connect(Connection, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) != 0) {
        std::cout << "Ошибка: неудачная попытка присоединится к серверу.\n";
        return 1;
    }
    std::cout << "Удачное присоединение к серверу!\n";

    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, NULL, NULL, NULL);

    bool sessionActive = true;
    while (sessionActive) {
        std::cout << "1 - Авторизация | 2 - Регистрация | 3 - Выйти" << std::endl;
        int select;
        std::cin >> select;
        switch (select) {
        case 1:
            login();
            std::cout << "Введите сообщение:" << std::endl;
            chat();
            break;
        case 2:
            regist();
            std::cout << "Для входа в систему, выберите 1 - Авторизация" << std::endl;
            break;
        case 3:
            sessionActive = false;
            break;
        default:
            std::cout << "Ошибка ввода: Выберите 1, 2 или 3" << std::endl;
            continue;
        }
    }

    mysql_close(conn);
    return 0;
}