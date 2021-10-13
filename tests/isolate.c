/*
 * Prown is a simple tool developed to give users the possibility to
 * own projects (files and repositories).
 * Copyright (C) 2021 EDF SA.

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sched.h>
#include <sys/mount.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#define ERROR(fmt, ...) \
        do { fprintf(stderr, "ERROR: %s:%d:%s(): " fmt, __FILE__, \
                      __LINE__, __func__, ##__VA_ARGS__); } while (0)
#define DEBUG(fmt, ...) \
        do { printf("DEBUG: %s(): " fmt, __func__, ##__VA_ARGS__); } while (0)

int init_namespace() {

    /*
     * Create a mount namespace to create a test environment based on the
     * current system without modification on this system.
     */

    DEBUG("creating mount namespace\n");
    if (unshare(CLONE_NEWNS)) { // requires CAP_SYS_ADMIN, see mount_namespaces(7)
        ERROR("unable to unshare(): %s\n", strerror(errno));
        return 1;
    }

    /*
     * Make / private to the namespace (ie. specific peer-group) to avoid
     * propagation of subsequents mounts on the system.
     */
    if (mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL)) {
        ERROR("error make / MS_PRIVATE: %s\n", strerror(errno));
        return 1;
    }

    /*
     * Create small tmpfs FS on arbitrary mount point /mnt. In this FS, create
     * mount point for an RW overlay over /.
     *
     *
     *   /              o-------+ (lower [RO])
     *   |                      |
     *   +- mnt/                |
     *       |                  |
     *     (tmpfs)              |
     *       |                  |
     *       +- root/   o-------+ (upper [RW])
     *       +- work/   o-------+ (workdir)
     *       +- merged/ <-------+
     */

    DEBUG("creating tmpfs\n");
    if (mount("tmpfs", "/mnt", "tmpfs", 0, "size=64M")) {
        ERROR("unable to create tmpfs: %s\n", strerror(errno));
        return 1;
    }

    /* mkdir 755 */
    if (mkdir("/mnt/merged", S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)) {
        ERROR("unable to mkdir %s: %s\n", "/mnt/root", strerror(errno));
        return 1;
    }

    if (mkdir("/mnt/root", S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)) {
        ERROR("unable to mkdir %s: %s\n", "/mnt/root", strerror(errno));
        return 1;
    }

    if (mkdir("/mnt/work", S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)) {
        ERROR("unable to mkdir %s: %s\n", "/mnt/work", strerror(errno));
        return 1;
    }

    DEBUG("creating overlay\n");
    if (mount("overlay", "/mnt/merged", "overlay", 0, "lowerdir=/,upperdir=/mnt/root,workdir=/mnt/work")) {
        ERROR("unable to mount overlayfs: %s\n", strerror(errno));
        return 1;
    }


    /* Then prepare the /mnt/merged for chroot with /proc and binded /tmp:
     *
     *   / o------------------o
     *   |                    |
     *   +- mnt/              |
     *   |   |            [2. chroot]
     *   | (tmpfs)            |
     *   |   |                |
     *   |   +- merged/  <----+
     *   |       |
     *   |       + proc/ <----o [3. procfs]
     *   |       + tmp/  <----+
     *   |                    | [1. bind]
     *   + tmp/  o------------+
     */
    DEBUG("bind-mount /tmp\n");
    if (mount("/tmp", "/mnt/merged/tmp", NULL, MS_BIND, NULL)) {
        ERROR("unable to bind-mount /tmp: %s\n", strerror(errno));
        return 1;
    }

    DEBUG("chroot\n");
    chroot("/mnt/merged");

    DEBUG("mounting /proc\n");
    if (mount("proc", "/proc", "proc", 0, NULL)) {
        ERROR("unable to mount /proc: %s\n", strerror(errno));
        return 1;
    }

}

int launch_tests() {

    char *env[] = { "PYTHONIOENCODING=utf-8", NULL };
    char *path;
    char *bin_path = malloc(PATH_MAX);

    /* Look for launch.py in the directory of the current binary */

    if(readlink("/proc/self/exe", bin_path, PATH_MAX) == -1) {
        ERROR("unable to readlink(): %s\n", strerror(errno));
        return 1;
    }
    path = dirname(bin_path);

    strncat(path, "/launch.py", PATH_MAX);

    char *argv[] = { "/usr/bin/python3", path, NULL };

    execve("/usr/bin/python3", argv, env);
    return 0;

}

int main(int argc, char * argv[]) {

    if (init_namespace())
      return EXIT_FAILURE;

    if (launch_tests())
      return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
