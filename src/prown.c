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

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <bsd/string.h>
#include <limits.h>
#include <ctype.h>
#include <dirent.h>
#include <pwd.h>
#include <error.h>
#include <getopt.h>
#include <grp.h>
#include <errno.h>
#include <stdbool.h>
#include <libintl.h>
#include <locale.h>
#include <acl/libacl.h>

#define MAXLINE  1000
#define _(STRING) gettext(STRING)

#define VERBOSE(fmt, ...) if(!verbose); else printf(fmt, ## __VA_ARGS__)
#define ERROR(fmt, ...) \
        do { fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)


/* static variable for verbose mode */
static int verbose;

/* number of project paths in config file */
static int nop = 0;

/**********************************************************
 *                                                        *
 *                  Configuration load                    *
 *                                                        *
 **********************************************************/

/*
 * Read lines from config file.
 * */
void read_str_from_config_line(char *config_line, char *val) {
    char prm_name[MAXLINE];

    sscanf(config_line, "%s %s\n", prm_name, val);
}

/*
 * Read config file. projects_parents is the lis of projects
 * */
void read_config_file(char config_filename[], char *projects_parents[]) {
    FILE *fp;
    char buf[MAXLINE];

    if ((fp = fopen(config_filename, "r")) == NULL) {
        ERROR(_("Failed to open configuration file %s\n"), config_filename);
        exit(EXIT_FAILURE);
    }
    while (!feof(fp)) {
        char *rc = fgets(buf, MAXLINE, fp);

        if (!feof(fp) && rc == NULL) {
            ERROR(_("Unable to read configuration file %s\n"),
                  config_filename);
            exit(EXIT_FAILURE);
        }
        if (buf[0] == '#' || strlen(buf) < 4) {
            continue;
        }
        if (strstr(buf, "PROJECT_DIR ")) {
            if ((projects_parents[nop] =
                 malloc(sizeof(char) * PATH_MAX)) == NULL) {
                ERROR(_
                      ("Unable to allocate memory for loading "
                       "configuration file parameters\n"));
                exit(1);
            }
            read_str_from_config_line(buf, projects_parents[nop]);
            nop++;
        }
    }
    fclose(fp);
}

/**********************************************************
 *                                                        *
 *                      Helpers                           *
 *                                                        *
 **********************************************************/

/*
 * Returns true if user is member of group whose gid is in a
 * argument, false otherwise.
 */
bool is_user_in_group(gid_t gid) {

    struct passwd *pw;
    int ngroups = 0;

    pw = getpwuid(getuid());

    if (pw == NULL) {
        perror(_("Error on getpwuid(): "));
    }
    //this call is just to get the correct ngroups
    getgrouplist(pw->pw_name, pw->pw_gid, NULL, &ngroups);
    __gid_t groups[ngroups];

    //here we actually get the groups
    getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);

    for (int i = 0; i < ngroups; i++) {
        if (groups[i] == gid) {
            VERBOSE(_("User is a valid member of group %s (%d)\n"),
                    getgrgid(gid)->gr_name, gid);
            return true;
        }
    }
    VERBOSE(_("User is NOT a valid member of group %s (%d)\n"),
            getgrgid(gid)->gr_name, gid);
    return false;
}

/*
 * Returns true if path is under one of the given projects_parents. If true,
 * project_parent string is set with the matching projects_parents.
 *
 * The project_parent argument must be a preallocated string.
 *
 * Examples:
 *
 *   With:
 *
 *     projects_parents = { '/projects', '/data' }
 *     path = '/projects/awesome/data'
 *
 *     → is_in_projects_parents() returs true and set project_parent to '/projects'.
 *
 *   With:
 *
 *     projects_parentts = { '/projects', '/data' }
 *     path = '/tmp/file'
 *
 *     → is_in_projects_parents() returns false (project_parent is not modified).
 */

bool is_in_projects_parents(char **projects_parents, char *project_parent,
                            const char *path) {

    for (int i = 0; i < nop; i++) {
        int l = strlen(projects_parents[i]);

        //if file in list of projects but not equal the project
        if ((strncmp(path, projects_parents[i], l) == 0)
            && (strcmp(path, projects_parents[i]))) {
            strncpy(project_parent, projects_parents[i], PATH_MAX);
            return true;
        }
    }

    return false;
}

/*
 * Set project_root with the path of the root directory of the project
 * located under project_parent and containing the path in argument.
 *
 * Example:
 *
 *    With:
 *
 *      project_parent = '/projects'
 *      path = '/projects/awesome/path/to/file'
 *
 *    → project_root is '/projects/awesome'
 */

void get_project_root(char *project_parent, char *path, char *project_root) {

    // get the path of project
    int l = strlen(project_parent);
    int h = l + 1;

    /* clean allocated memory for strings */
    memset(project_root, 0, PATH_MAX);

    while (path[h] != '\0' && path[h] != '/') {
        h++;
    }
    strcpy(project_root, project_parent);
    /* concat with the basename of project directory */
    strncat(project_root, &path[l], h - l);

    VERBOSE(_("Project path: %s\n"), project_root);

}

/*
 * Returns true if ACL entry is of type group, false otherwise.
 */

bool check_acl_is_group(acl_entry_t ent) {

    acl_tag_t e_type;

    if (acl_get_tag_type(ent, &e_type) == -1) {
        perror(_("Error on acl_get_tag_type()"));
        return false;
    }

    return (e_type == ACL_GROUP);

}

/*
 * Returns true if ACL entry has write permission, false otherwise.
 */

bool check_acl_can_write(acl_entry_t ent) {

    acl_permset_t e_permset;
    int perm;

    if (acl_get_permset(ent, &e_permset) == -1) {
        perror(_("Error on acl_get_permset()"));
        return false;
    }

    perm = acl_get_perm(e_permset, ACL_WRITE);

    if (perm == -1) {
        perror(_("Error on acl_get_perm()"));
        return false;
    }

    return (perm == 1);
}

/*
 * Returns true if current user is member ACL entry GID (considering it is has
 * group type, and must be checked before call this function), false otherwise.
 */

bool check_acl_user_in_group(acl_entry_t ent) {

    gid_t *id_p = acl_get_qualifier(ent);

    if (!id_p) {
        perror(_("Error on acl_get_qualifier()"));
        return false;
    }

    return is_user_in_group(*id_p);
}


/*
 * Returns true if the ACL entry fulfills all requirements:
 *  - it has the group type
 *  - it has the write permission
 *  - the current user is member of this group
 *
 * It returns false if any of the above is false, or if any rerror is
 * encountered.
 */

bool check_acl_entry(acl_entry_t ent) {

    if (!check_acl_is_group(ent)) {
        return false;
    }

    if (!check_acl_can_write(ent)) {
        return false;
    }

    return check_acl_user_in_group(ent);

}

/*
 * Returns true if the current user is a valid administrator of the project,
 * ie. she/he is a member of the project group owner of the project root
 * directory.
 */

bool is_user_project_admin(const char *project_root) {

    struct stat sb;
    bool is_admin = false;      /* return value */
    acl_t acl;
    acl_entry_t ent;
    int ret;

    /* Get gid of group owner of project root directory */
    if (stat(project_root, &sb) == -1) {
        perror(_("Error on stat()"));
        exit(EXIT_FAILURE);
    }

    VERBOSE(_("Project group owner: %s (%d)\n"), getgrgid(sb.st_gid)->gr_name,
            sb.st_gid);

    /* Return true if user is member of group owner of project root directory */
    if (is_user_in_group(sb.st_gid)) {
        return true;
    }

    VERBOSE(_("Checking ACL\n"));

    acl = acl_get_file(project_root, ACL_TYPE_ACCESS);

    if (acl == NULL) {
        perror(_("Error on acl_get_file()"));
        return false;
    }

    ret = acl_get_entry(acl, ACL_FIRST_ENTRY, &ent);

    if (ret == -1) {
        perror(_("Error on acl_get_entry()"));
        goto end;
    }

    if (ret == 0) {
        VERBOSE(_("No ACL entries available\n"));
        goto end;
    }

    while (ret > 0) {
        if (check_acl_entry(ent)) {
            is_admin = true;
            goto end;
        }
        ret = acl_get_entry(acl, ACL_NEXT_ENTRY, &ent);
    }

  end:
    acl_free(acl);
    return is_admin;

}

/**********************************************************
 *                                                        *
 *             Workflow processing functions              *
 *                                                        *
 **********************************************************/

/*set user as the owner of the current file or directory*/
void setOwner(const char *path) {
    //use lchown to change owner for symlinks
    VERBOSE(_("Changing owner of path %s\n"), path);
    if (lchown(path, getuid(), (gid_t) - 1) != 0) {
        perror(_("Error on chown(): "));
        exit(EXIT_FAILURE);
    }
    //set rw to group if its not a symlink
    struct stat buf;

    if (lstat(path, &buf)) {
        perror(_("Error on lstat()"));
        exit(EXIT_FAILURE);
    }

    if (!S_ISLNK(buf.st_mode)) {
        struct stat st;

        VERBOSE(_("Ensuring group owner has rw permissions on path %s\n"),
                path);

        stat(path, &st);
        if (chmod(path, S_IRGRP | S_IWGRP | st.st_mode) != 0) {
            perror(_("Error on chmod(): "));
            exit(EXIT_FAILURE);
        }
    }
}

/*
 * set recursively the user as the owner of the project.
 *
 * Returns 0 if valid, 1 otherwise.
 * */
int projectOwner(char *basepath) {
    struct stat buf;

    lstat(basepath, &buf);
    int status = 0;

    if (S_ISDIR(buf.st_mode)) {
        char path[PATH_MAX];
        struct dirent *dp;
        DIR *dir = opendir(basepath);

        // Unable to open directory stream
        if (!dir) {
            ERROR(_("Failed to open directory '%s': %s (%d)\n"), basepath,
                  strerror(errno), errno);
            return 1;
        }

        VERBOSE(_("Changing recursively owner of directory %s content\n"),
                basepath);

        while ((dp = readdir(dir)) != NULL) {
            if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0
                && strcmp(dp->d_name, basepath) != 0) {
                // Construct new path from our base path
                if (strlcpy(path, basepath, sizeof(path)) >= sizeof(path))
                    exit(1);
                strcat(path, "/");
                strcat(path, dp->d_name);
                setOwner(path);
                projectOwner(path);
            }
        }
        closedir(dir);
    }
    return status;
}

int prownProject(char *path) {
    char *projects_parents[PATH_MAX];
    char real_dir[PATH_MAX];
    bool isInProjectPath;
    char project_parent[PATH_MAX], project_root[PATH_MAX];
    struct stat path_stat;


    /* clean allocated memory for strings */
    memset(real_dir, 0, PATH_MAX);
    memset(project_parent, 0, PATH_MAX);
    memset(project_root, 0, PATH_MAX);

    read_config_file("/etc/prown.conf", projects_parents);

    VERBOSE(_("+ Processing path %s\n"), path);

    // check the real path is correct
    if (!realpath(path, real_dir)) {
        ERROR(_("Path '%s' has not been found, it is discarded\n"), path);
        return 0;
    }

    /* check path is under projects roots directories */
    isInProjectPath =
        is_in_projects_parents(projects_parents, project_parent, real_dir);

    if (!isInProjectPath) {
        ERROR(_("Changing owner of file outside project parent directories "
                "is prohibited, path '%s' is discarded\n"), path);
        return 0;
    }

    /* get project root directory */
    get_project_root(project_parent, real_dir, project_root);

    /* check user is administrator of this project */
    if (!is_user_project_admin(project_root)) {
        ERROR(_("Permission denied for project %s, you are not a member of "
                "this project administor groups\n"), project_root);
        return 0;
    } else
        VERBOSE(_("User is granted to prown in project directory %s\n"),
                project_root);

    stat(real_dir, &path_stat);
    //if it's a file we should call setOwner one time
    if (path_stat.st_mode & S_IFREG) {
        setOwner(real_dir);
    } else {
        // chown the real_dir if it's not the projectDir
        // because projectOwner() doesn't chown the entry path
        // but only the chlids
        if (strcmp(real_dir, project_root))
            setOwner(real_dir);
        projectOwner(real_dir);
    }

    return 0;
}

/**********************************************************
 *                                                        *
 *                        CLI                             *
 *                                                        *
 **********************************************************/

void usage(int status) {
    if (status != EXIT_SUCCESS)
        printf(_("Try 'prown --help' for more information.\n"));
    else {
        printf(_("Usage: prown [OPTION]... PATH...\n"
                 "Give user ownership of PATH in project directories. If the "
                 "PATH is a directory,\nit gives user ownership of all files "
                 "in this directory recursively.\n"
                 "\n"
                 "  -v, --verbose          Display modified paths and more "
                 "information\n"
                 "  -h, --help             Display this help and exit\n"
                 "\n"
                 "The user must be a member of project administrator groups "
                 "to take ownership of\npath in a project directory.\n"
                 "\n"
                 "Examples :\n"
                 "  prown awesome/data     Take ownership of data file in "
                 "awesome\n"
                 "                         project directory\n"
                 "  prown awesome          Take ownership of awesome "
                 "project\n"
                 "                         directory recursively\n"
                 "  prown awesome crazy    Take ownership of both awesome "
                 "and crazy\n"
                 "                         project directories recursively\n"));
    }
}

int main(int argc, char **argv) {
    char *options = "hv";
    int longindex;
    int opt;
    int help = 0;

    struct option longopts[] = {
        {"help", no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    /* Setting the i18n environment */
    setlocale(LC_ALL, "");
    bindtextdomain("prown", "/usr/share/locale/");
    textdomain("prown");

    while ((opt =
            getopt_long(argc, argv, options, longopts, &longindex)) != -1) {
        switch (opt) {
        case 'v':
            verbose++;
            break;
        case 'h':
            help = 1;
            usage(EXIT_SUCCESS);
            break;
        default:
            usage(EXIT_FAILURE);
            break;
        }
    }
    if ((argc == 1 || optind == argc) && (help != 1)) {
        error(0, 0, _("Missing path operand"));
        usage(EXIT_FAILURE);
    } else {
        for (; optind < argc; optind++) {
            char *path = argv[optind];

            prownProject(path);
        }
    }
}
