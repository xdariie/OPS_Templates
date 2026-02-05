ssize_t read(int fd, void *buf, size_t count);
//fd – file descriptor
//buf – buffer to store data
//count – max bytes to read
//Reads bytes from a file descriptor.


ssize_t write(int fd, const void *buf, size_t count);
//Writes bytes to a file descriptor.


TEMP_FAILURE_RETRY(read(...))
//Automatically retries a syscall if interrupted by a signal (EINTR).


int open(const char *path, int flags, mode_t mode);
//Opens or creates a file, returns file descriptor.


int close(int fd);
//Closes file descriptor.


off_t lseek(int fd, off_t offset, int whence);
//Moves file offset (SEEK_SET, SEEK_CUR, SEEK_END).


pid_t fork(void);
//Creates a new process.
// 0 → child
// >0 → parent
// -1 → error


pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
//Waits for child process termination.


int kill(pid_t pid, int sig);
//Sends a signal to a process or process group.


int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);
//Installs a signal handler (POSIX-safe).


struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
};


void sig_handler(int sig);
//Handles received signal, usually sets a flag.


int sigemptyset(sigset_t *set);
//Initializes empty signal set.


int sigaddset(sigset_t *set, int sig);
//Adds signal to set.


int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
//Blocks or unblocks signals.


int sigsuspend(const sigset_t *mask);
//Temporarily replaces signal mask and waits for signal.