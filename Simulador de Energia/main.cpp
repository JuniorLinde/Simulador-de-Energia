#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <iomanip>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

void initialize_winsock() {
#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup falhou: " << iResult << std::endl;
        exit(1);
    }
#endif
}

void cleanup_winsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

std::string generate_production_data(int hour, int second) {
    std::map<std::string, std::pair<double, double>> sources_data;

    sources_data["Solar"] = {
        100.0 + (hour * 5.0) + (second * 0.1),
        0.75 + (hour * 0.01) - (second * 0.001)
    };
    sources_data["Eolica"] = {
        150.0 + (hour * 7.0) - (second * 0.2),
        0.80 - (hour * 0.005) + (second * 0.002)
    };
    sources_data["Hidro"] = {
        200.0 - (hour * 3.0) + (second * 0.05),
        0.90 + (hour * 0.002) - (second * 0.0005)
    };
    sources_data["Biomassa"] = {
        80.0 + (hour * 2.0) + (second * 0.05),
        0.65 + (hour * 0.008) - (second * 0.0003)
    };
    sources_data["Nuclear"] = {
        300.0 - (hour * 1.0) + (second * 0.01),
        0.95 - (hour * 0.001) + (second * 0.0001)
    };
    sources_data["Termica"] = {
        250.0 + (hour * 4.0) - (second * 0.15),
        0.70 + (hour * 0.003) - (second * 0.0008)
    };

    double total_production = 0.0;
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);

    for (const auto& pair : sources_data) {
        ss << pair.first << ":" << pair.second.first << "," << pair.second.second << ";";
        total_production += pair.second.first;
    }

    ss << "total:" << total_production;
    return ss.str();
}

int main() {
    initialize_winsock();

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    // Alterado o tamanho do buffer para 1025 para acomodar o terminador nulo
    char buffer[1025] = { 0 };
    const int PORT = 12345;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket falhou");
        cleanup_winsock();
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt))) {
        perror("setsockopt");
        cleanup_winsock();
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind falhou");
        cleanup_winsock();
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        cleanup_winsock();
        exit(EXIT_FAILURE);
    }

    std::cout << "Servidor C++ escutando na porta " << PORT << std::endl;
    std::cout << "Aguardando conexão do cliente Python..." << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (int*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        std::cout << "Cliente Python conectado!" << std::endl;

        // O recv ainda lê no máximo 1024 bytes, garantindo espaço para o '\0' no buffer[1024]
        int valread = recv(new_socket, buffer, 1024, 0);
        if (valread <= 0) {
            std::cerr << "Falha ao ler do cliente ou cliente desconectado." << std::endl;
#ifdef _WIN32
            closesocket(new_socket);
#else
            close(new_socket);
#endif
            continue;
        }

        buffer[valread] = '\0'; // Agora seguro, pois buffer tem 1025 posições
        std::string client_message(buffer);
        std::cout << "Recebido do Python: " << client_message << std::endl;

        int hour = 0;
        int second = 0;
        try {
            size_t hour_pos = client_message.find("hora:");
            size_t second_pos = client_message.find("segundo:");
            if (hour_pos != std::string::npos && second_pos != std::string::npos) {
                std::string hour_str = client_message.substr(hour_pos + 5, second_pos - (hour_pos + 5) - 1);
                std::string second_str = client_message.substr(second_pos + 8);
                hour = std::stoi(hour_str);
                second = std::stoi(second_str);
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Erro ao analisar a mensagem do cliente: " << e.what() << std::endl;
        }

        std::string response = generate_production_data(hour, second);
        send(new_socket, response.c_str(), response.length(), 0);
        std::cout << "Enviado para o Python: " << response << std::endl;

#ifdef _WIN32
        closesocket(new_socket);
#else
        close(new_socket);
#endif
        std::cout << "Cliente desconectado." << std::endl;
    }

#ifdef _WIN32
    closesocket(server_fd);
#else
    close(server_fd);
#endif
    cleanup_winsock();
    return 0;
}
