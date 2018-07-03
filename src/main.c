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
    uid_t ruid, euid, suid; /* Real, Effective, Saved user ID */
    gid_t rgid, egid, sgid; /* Real, Effective, Saved group ID */
    int uerr, gerr, fd;

    if (getresuid(&ruid, &euid, &suid) == -1) {
        fprintf(stderr, "Cannot obtain user identity: %m.\n");
        return TFTPSTATS_FAILURE;
    }
    if (getresgid(&rgid, &egid, &sgid) == -1) {
        fprintf(stderr, "Cannot obtain group identity: %m.\n");
        return TFTPSTATS_FAILURE;
    }
    if (ruid != (uid_t) TARGET_UID && ruid < (uid_t) MIN_UID) {
        fprintf(stderr, "Invalid user.\n");
        return TFTPSTATS_FAILURE;
    }
    if (rgid != (gid_t) TARGET_UID && rgid < (gid_t) MIN_GID) {
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

    /* ... privileged operations ... */

    /* Open the restricted file.
     * If 'filename' is specified by the calling user,
     * in command-line parameters or environment variables,
     * you may have handed them a way to read e.g. /etc/gpasswd.
     * Don't do that.
     * If you have to do that, a lot of additional checks are needed,
     * some before opening the file, and others after opening the file.
     * Even then it is risky (hard link races and such).
    */

    /* Drop privileges. */
    gerr = 0;
    if (setresgid(rgid, rgid, rgid) == -1) {
        gerr = errno;
        if (!gerr)
            gerr = EINVAL;
    }
    uerr = 0;
    if (setresuid(ruid, ruid, ruid) == -1) {
        uerr = errno;
        if (!uerr)
            uerr = EINVAL;
    }
    if (uerr || gerr) {
        if (uerr)
            fprintf(stderr, "Cannot drop user privileges: %s.\n",
                    strerror(uerr));
        if (gerr)
            fprintf(stderr, "Cannot drop group privileges: %s.\n",
                    strerror(gerr));
        return TFTPSTATS_FAILURE;
    }

    /* ... unprivileged operations ... */
    
    return 0;
}
