//
// Filename: main
// Created by: louiscyphre@github
// Created on: 20:12 03.07.2018
//

#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#ifndef TARGET_UID
    #define TARGET_UID 0
#endif

#ifndef TARGET_GID
    #define TARGET_GID 0
#endif

#ifndef MIN_UID
    #define MIN_UID 500
#endif

#ifndef MIN_GID
    #define MIN_GID 500
#endif

// Types ///////////////////////////////////////////////////////////////////////

typedef enum {
    TFTPSTATS_FAILURE = -1, TFTPSTATS_SUCCESS, TFTPSTATS_INVALID_PARAMETER,
    TFTPSTATS_ERROR_GETADDRINFO, TFTPSTATS_COULD_NOT_CONNECT,
    TFTPSTATS_ERROR_SENDTO, TFTPSTATS_UNEXPECTED_PACKET,
    TFTPSTATS_ERROR_RECVFROM, TFTPSTATS_PACKET_SENT_PARTIALLY,
    TFTPSTATS_ERROR_SELECT, TFTPSTATS_TIMEOUT,
    TFTPSTATS_ERROR_OPEN, TFTPSTATS_MAX_TIMEOUTS_REACHED,
    TFTPSTATS_MAX_INVALID_PACKETS_REACHED,
    TFTPSTATS_FILE_NAME_TOO_LONG, TFTPSTATS_MODE_NAME_TOO_LONG,
    TFTPSTATS_OUT_OF_MEMORY
} tftpstats_result_t;

int main(int argc, char *argv[]) {
    uid_t real_uid, effective_uid, saved_uid;/* Real, Effective, Saved user * ID */
    gid_t real_gid, effective_gid, saved_gid; /* Real, Effective, Saved group * ID */
    int uid_error, gid_error;

    if (getresuid(&real_uid, &effective_uid, &saved_uid) == -1) {
        fprintf(stderr, "Cannot obtain user identity: %s.\n", strerror(errno));
        return TFTPSTATS_FAILURE;
    }
    if (getresgid(&real_gid, &effective_gid, &saved_gid) == -1) {
        fprintf(stderr, "Cannot obtain group identity: %s.\n", strerror(errno));
        return TFTPSTATS_FAILURE;
    }
    if (real_uid != (uid_t) TARGET_UID && real_uid < (uid_t) MIN_UID) {
        fprintf(stderr, "Invalid user.\n");
        return TFTPSTATS_FAILURE;
    }
    if (real_gid != (gid_t) TARGET_UID && real_gid < (gid_t) MIN_GID) {
        fprintf(stderr, "Invalid group.\n");
        return TFTPSTATS_FAILURE;
    }

    /* Switch to target user. setuid bit handles this, but doing it again does no harm. */
    if (seteuid((uid_t) TARGET_UID) == -1) {
        fprintf(stderr, "Insufficient user privileges.\n");
        return TFTPSTATS_FAILURE;
    }

    /* Switch to target group. setgid bit handles this, but doing it again does no harm.
     * If TARGET_UID == 0, we need no setgid bit, as root has the privilege. */
    if (setegid((gid_t) TARGET_GID) == -1) {
        fprintf(stderr, "Insufficient group privileges.\n");
        return TFTPSTATS_FAILURE;
    }

    pid_t pid;
    if (pid = fork() < 0) {
        fprintf(stderr, "[ERROR] fork(): .\n", strerror(errno));
        return TFTPSTATS_FAILURE;
    } else if (pid == 0) {
        /* Drop privileges for child process. */
        gid_error = 0;
        if (setresgid(real_gid, real_gid, real_gid) == -1) {
            gid_error = errno;
            if (!gid_error)
                gid_error = EINVAL;
        }
        uid_error = 0;
        if (setresuid(real_uid, real_uid, real_uid) == -1) {
            uid_error = errno;
            if (!uid_error)
                uid_error = EINVAL;
        }
        if (uid_error || gid_error) {
            if (uid_error)
                fprintf(stderr, "Cannot drop user privileges: %s.\n",
                        strerror(uid_error));
            if (gid_error)
                fprintf(stderr, "Cannot drop group privileges: %s.\n",
                        strerror(gid_error));
            return TFTPSTATS_FAILURE;
        }
        
    }
    /* ... privileged operations ... */




    /* ... unprivileged operations ... */

    return 0;
}
