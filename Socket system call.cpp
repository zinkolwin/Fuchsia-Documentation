#include <fuchsia/posix/socket/cpp/fidl.h>
#include <lib/async-loop/cpp/loop.h>
#include <lib/async-loop/default.h>
#include <lib/fidl/cpp/binding_set.h>
#include <lib/sys/cpp/component_context.h>
#include <lib/zx/socket.h>
#include <lib/fdio/fd.h>
#include <lib/fdio/io.h>
#include <unistd.h>
#include <iostream>

int main() {
    // Start the event loop
    async::Loop loop(&kAsyncLoopConfigAttachToCurrentThread);

    // Get the ComponentContext to access services
    auto context = sys::ComponentContext::CreateAndServeOutgoingDirectory();

    // Connect to the socket provider
    fuchsia::posix::socket::ProviderPtr socket_provider;
    context->svc()->Connect(socket_provider.NewRequest());

    // Create a TCP socket (IPv4)
    zx::socket socket_handle;
    int32_t err;
    socket_provider->Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP, &socket_handle, &err);

    if (err != 0) {
        std::cerr << "Failed to create socket: " << err << std::endl;
        return 1;
    }

    // Convert zx::socket to a POSIX file descriptor using fdio
    int fd;
    zx_status_t status = fdio_fd_create(socket_handle.release(), &fd);
    if (status != ZX_OK) {
        std::cerr << "fdio_fd_create failed: " << status << std::endl;
        return 1;
    }

    // Define server address to connect to
    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080); // Example port
    server_addr.sin_addr.s_addr = htonl(0x7f000001); // 127.0.0.1

    // Connect to the server
    if (connect(fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        perror("Connect failed");
        close(fd);
        return 1;
    }

    // Send a message
    const char* message = "Hello from Fuchsia!";
    send(fd, message, strlen(message), 0);

    // Receive response
    char buffer[256] = {};
    recv(fd, buffer, sizeof(buffer), 0);
    std::cout << "Received: " << buffer << std::endl;

    // Close the socket
    close(fd);
    return 0;
}
