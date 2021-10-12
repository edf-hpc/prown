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

// Maximum number of users per group
#define MAX_USERS 10

// Maximum number of groups
#define MAX_GROUPS 10

// Maximum length of user names
#define MAX_USER_LEN 30

// Maximum length of group names
#define MAX_GROUP_LEN 30

// Maximum length of user lists in /etc/group lines
const uint16_t MAX_USERS_LEN = MAX_USERS * MAX_USER_LEN;

// Maximum size of line in /etc/passwd
const uint16_t MAX_USERLINE_LEN = MAX_USER_LEN + 20;

// Maximum size of line in /etc/group
const uint16_t MAX_GROUPLINE_LEN = MAX_GROUP_LEN + 5 + MAX_USERS * MAX_USER_LEN;

uid_t next_uid = 100001;
gid_t next_gid = 100001;

typedef struct userl_s {
  char *name;
  uid_t uid;
  struct userl_s *next;
} userl_t;

typedef struct groupl_s {
  char *name;
  gid_t gid;
  userl_t *users;
  struct groupl_s *next;
} groupl_t;

int db_add_group(groupl_t **groups, char *groupname) {

    groupl_t *last_group = *groups, *new_group = NULL;
    uint16_t id_group = 0;

    /* go to end of group */
    while(last_group) {
        if(++id_group == MAX_GROUPS) {
            ERROR("MAX_GROUPS %u is reached, unable to add group\n", MAX_GROUPS);
            return 1;
        }
        if(!last_group->next)
            break;
        last_group = last_group->next;
        id_group++;
    }

    new_group = (groupl_t *)malloc(sizeof(groupl_t));
    new_group->name = groupname;
    new_group->gid = next_gid++;
    new_group->users = NULL;
    new_group->next = NULL;
    /* if 1st group */
    if(!*groups)
        *groups = new_group;
    else
        last_group->next = new_group;

    DEBUG("add group %u: %s (%u)\n", id_group, new_group->name, new_group->gid);

    return 0;
}

int db_add_user_in_group(groupl_t *groups, const char *groupname, char *username) {

    groupl_t *group = groups;
    uint16_t id_group = 0;
    userl_t *last_user = NULL, *new_user = NULL;
    uint16_t id_user = 0;

    if (!group) {
        ERROR("group list is empty\n");
        return 1;
    }

    /* find the group */
    while(group) {
        if(++id_group == MAX_GROUPS) {
            ERROR("MAX_GROUPS %u is reached, unable to find group %s\n", groupname);
            return 1;
        }

        // group match
        if(strncmp(group->name, groupname, strlen(groupname)))
            break;

        if(!group->next) {
            ERROR("group list end is reached, unable to find group %s\n", groupname);
            return 1;
        }

        group = group->next;
    }

    last_user = group->users;

    /* go to end of group users list */
    while(last_user) {
        if(++id_user == MAX_USERS) {
            ERROR("MAX_USERS %u is reached, unable to add user\n", MAX_USERS);
            return 1;
        }
        if(!last_user->next)
            break;
        last_user = last_user->next;
    }

    new_user = (userl_t *)malloc(sizeof(userl_t));
    new_user->name = username;
    new_user->uid = next_uid++;
    new_user->next = NULL;
    /* if 1st user in group */
    if(!group->users)
        group->users = new_user;
    else
        last_user->next = new_user;

    DEBUG("add user %u: %s (%u)\n", id_user, new_user->name, new_user->uid);

    return 0;

}

int append_groups(const char *path, groupl_t *groups) {

    int path_fd;
    size_t write_l;
    char users[MAX_USERS_LEN];
    char line[MAX_GROUPLINE_LEN];
    groupl_t *group = groups;
    uint16_t id_group = 0, id_user = 0;
    userl_t *user = NULL;

    path_fd = open(path, O_APPEND | O_WRONLY);
    if (path_fd == -1) {
        ERROR("unable to open() %s\n", path, strerror(errno));
        return 1;
    }

    while(group) {

        assert(id_group++<MAX_GROUPS);

        id_user = 0;
        memset(users, 0, MAX_USERS_LEN); // clear string

        user = group->users;
        /* construct users list string */
        while(user) {
            assert(id_user++<MAX_USERS);

            if(strlen(users))
                strncat(users, ",", 1);
            strncat(users, user->name, MAX_USER_LEN);
            user = user->next;
        }

        snprintf(line, MAX_GROUPLINE_LEN, "%s:x:%u:%s\n", group->name, group->gid, users);
        DEBUG("new group line %s", line);

        write_l = write(path_fd, line, strlen(line));
        if (!write_l) {
            ERROR("unable to write() %s\n", path);
            close(path_fd);
            return 1;
        }

        group = group->next;
    }

    close(path_fd);
    return 0;
}

int append_users(const char *path, groupl_t *groups) {

    int path_fd;
    size_t write_l;
    char line[MAX_USERLINE_LEN];
    groupl_t *group = groups;
    uint16_t id_group = 0, id_user = 0;
    userl_t *user = NULL;

    path_fd = open(path, O_APPEND | O_WRONLY);
    if (path_fd == -1) {
        ERROR("unable to open() %s\n", path, strerror(errno));
        return 1;
    }

    while(group) {

        assert(id_group++<MAX_GROUPS);

        id_user = 0;

        user = group->users;
        while(user) {
            assert(id_user++<MAX_USERS);

            snprintf(line, MAX_USERLINE_LEN,
                     "%s:x:%u:%u::/tmp:/bin/false\n",
                     user->name,
                     user->uid,
                     group->gid);

            DEBUG("new user line %s", line);

            write_l = write(path_fd, line, strlen(line));
            if (!write_l) {
                 ERROR("unable to write() %s\n", path);
                 close(path_fd);
                 return 1;
            }
            user = user->next;
        }
        group = group->next;

    }

    close(path_fd);
    return 0;

}

int init_namespace(groupl_t *groups) {

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

    /* add groups in copy of /etc/group */
    append_groups("/etc/group", groups);

    /* add users in copy of /etc/passwd */
    append_users("/etc/passwd", groups);

}

int launch_tests() {

    char *env[] = { NULL };
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

    groupl_t *groups = NULL;

    /* Build groups/users DB in memory */

    if (db_add_group(&groups, "biology") ||
        db_add_group(&groups, "physic") ||
        db_add_user_in_group(groups, "physic", "ana") ||
        db_add_user_in_group(groups, "physic", "mike") ||
        db_add_user_in_group(groups, "biology", "john") ||
        db_add_user_in_group(groups, "biology", "marie"))
      return EXIT_FAILURE;

    if (init_namespace(groups))
      return EXIT_FAILURE;

    if (launch_tests())
      return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
