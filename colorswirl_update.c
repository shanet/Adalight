#include <errno.h>
#include <mqueue.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    mqd_t mqd;

    // Limit the number of arguments to two digits for the buffer size below
    if(argc >= 100) {
        fprintf(stderr, "%s: Too many arguments.\n", argv[0]);
        return 1;
    }
    // Set the max message length as MAX_MSG_LEN
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = MAX_MSG_LEN,
        .mq_curmsgs = 0
    };

    // Create and open the message queue
    if((mqd = mq_open(MQ_NAME, O_WRONLY | O_CREAT, 0600, &attr)) == -1) {
        fprintf(stderr, "%s: Error opening queue: %s\n", argv[0], strerror(errno));
        return 1;
    }
    // Convert argc to a string and send it
    char _argc[2];
    snprintf(_argc, 2, "%d", argc);

    // Send the number of arguments
    if(mq_send(mqd, _argc, 2, 1) == -1) {
        fprintf(stderr, "%s: Failed to send argument count: %s\n", argv[0], strerror(errno));
        return 1;
    }

    // Flatten the argv array
    size_t argv_len = 0;
    char *argv_flat = malloc(MAX_MSG_LEN);
    for(int i=0; i<argc; i++) {
        strncat(argv_flat, argv[i], argv_len += strlen(argv[i]));

        // Add the space between arguments if not last argument
        if(i != argc-1) {
            strncat(argv_flat, " ", 1);
            argv_len++;
        }
    }

    // Check the argument string doesn't exceed the max length
    if(argv_len>= MAX_MSG_LEN) {
        fprintf(stderr, "%s: Arguments exceeds max length limit of %d\n", argv[0], MAX_MSG_LEN);
        return 1;
    }

    // Send the arguments
    if(mq_send(mqd, argv_flat, argv_len, 0) == -1) {
        fprintf(stderr, "%s: Failed to send arguments: %s\n", argv[0], strerror(errno));
        return 1;
    }

    // Close the queue
    mq_close(mqd);

    // Clean up
    free(argv_flat);
    argv_flat = NULL;

    return 0;
}
