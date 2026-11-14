

#define _POSIX_C_SOURCE 200809L

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static volatile sig_atomic_t got_hup = 0;

static void on_hup(int) {
    got_hup = 1;
}

static bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    return flags != -1 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

int main() {
    const unsigned short port = 9090;

    sigset_t blocked;
    sigset_t previous_mask;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blocked, &previous_mask) == -1) {
        std::perror("sigprocmask");
        return 1;
    }

    struct sigaction action{};
    action.sa_handler = on_hup;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGHUP, &action, nullptr) == -1) {
        std::perror("sigaction");
        return 1;
    }

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        std::perror("socket");
        return 1;
    }
    if (server >= FD_SETSIZE) {
        std::fputs("Слишком большой номер сокета\n", stderr);
        close(server);
        return 1;
    }

    int reuse = 1;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1 ||
        bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1 ||
        listen(server, 16) == -1 ||
        !set_nonblocking(server)) {
        std::perror("server setup");
        close(server);
        return 1;
    }

    sigset_t wait_mask = previous_mask;
    sigdelset(&wait_mask, SIGHUP);

    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("Сервер слушает порт %hu, PID %ld\n", port, static_cast<long>(getpid()));

    int client = -1;
    int exit_code = 0;
    for (;;) {
        fd_set ready_fds;
        FD_ZERO(&ready_fds);
        FD_SET(server, &ready_fds);
        if (client != -1) FD_SET(client, &ready_fds);

        int max_fd = client > server ? client : server;
        int ready = pselect(max_fd + 1, &ready_fds, nullptr, nullptr, nullptr, &wait_mask);
        int select_error = ready == -1 ? errno : 0;

        if (got_hup) {
            got_hup = 0;
            std::puts("Получен SIGHUP");
        }
        if (ready == -1) {
            if (select_error == EINTR) continue;
            errno = select_error;
            std::perror("pselect");
            exit_code = 1;
            break;
        }

        if (client != -1 && FD_ISSET(client, &ready_fds)) {
            char data[4096];
            ssize_t count = recv(client, data, sizeof(data), 0);
            if (count > 0) {
                std::printf("Получено байт: %zd\n", count);
            } else if (count == 0) {
                std::puts("Клиент отключился");
                close(client);
                client = -1;
            } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                std::perror("recv");
                close(client);
                client = -1;
            }
        }

        if (FD_ISSET(server, &ready_fds)) {
            for (;;) {
                int incoming = accept(server, nullptr, nullptr);
                if (incoming == -1) {
                    if (errno == EINTR || errno == ECONNABORTED) continue;
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    std::perror("accept");
                    exit_code = 1;
                    break;
                }

                std::puts("Новое подключение");
                if (client == -1 && incoming < FD_SETSIZE && set_nonblocking(incoming)) {
                    client = incoming;
                } else {
                    close(incoming);
                    std::puts("Подключение закрыто");
                }
            }
            if (exit_code != 0) break;
        }
    }

    if (client != -1) close(client);
    close(server);
    return exit_code;
}


