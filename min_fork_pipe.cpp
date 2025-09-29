//min_fork_pipe.cpp
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <unistd.h>     // pipe, fork, write, read, close, getpid
#include <sys/wait.h>   // waitpid
#include <cstring>      // strerror
#include <cerrno>

int main() {
    const int N = 20;
    std::vector<int> a(N);

    // Random numbers [0, 999]
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 999);
    for (int i = 0; i < N; ++i) a[i] = dist(gen);

    // Show the array
    std::cout << "Array (20 elems): ";
    for (int v : a) std::cout << v << " ";
    std::cout << "\n";

    int fds[2];
    if (pipe(fds) == -1) {
        std::cerr << "pipe() failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "fork() failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    if (pid == 0) {
        // Child: compute min of second half [10..19], send to parent
        close(fds[0]); // close read end
        int childMin = *std::min_element(a.begin() + N/2, a.end());
        std::cout << "[Child] PID=" << getpid()
                  << " | Min(second half, idx 10..19) = " << childMin << "\n";
        if (write(fds[1], &childMin, sizeof(childMin)) != sizeof(childMin)) {
            std::cerr << "[Child] write() failed: " << std::strerror(errno) << "\n";
        }
        close(fds[1]);
        _exit(0);
    } else {
        // Parent: compute min of first half [0..9], read child's min, combine
        close(fds[1]); // close write end
        int parentMin = *std::min_element(a.begin(), a.begin() + N/2);
        std::cout << "[Parent] PID=" << getpid()
                  << " | Min(first half, idx 0..9) = " << parentMin << "\n";

        int childMin = 0;
        ssize_t n = read(fds[0], &childMin, sizeof(childMin));
        if (n != sizeof(childMin)) {
            std::cerr << "[Parent] read() failed/short: " << std::strerror(errno) << "\n";
            childMin = parentMin; // fallback
        }
        close(fds[0]);

        int overall = std::min(parentMin, childMin);
        std::cout << "[Parent] Combined: min(parent=" << parentMin
                  << ", child=" << childMin << ") = " << overall << "\n";

        int status = 0;
        waitpid(pid, &status, 0);
        std::cout << "[Parent] Child PID " << pid << " exited with status " << status << "\n";
    }
    return 0;
}
