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
 * Read config file. projectsdir is the lis of projects
 * */
void read_config_file(char config_filename[], char *projectsdir[]) {
    FILE *fp;
    char buf[MAXLINE];

    if ((fp = fopen(config_filename, "r")) == NULL) {
        ERROR(_("Failed to open configuration file %s \n"), config_filename);
        exit(EXIT_FAILURE);
    }
    while (!feof(fp)) {
        fgets(buf, MAXLINE, fp);
        if (buf[0] == '#' || strlen(buf) < 4) {
            continue;
        }
        if (strstr(buf, "PROJECT_DIR ")) {
            if ((projectsdir[nop] = malloc(sizeof(char) * PATH_MAX)) == NULL) {
                ERROR(_("Unable to allocate memory for loading "
                        "configuration file parameters\n"));
                exit(1);
            }
            read_str_from_config_line(buf, projectsdir[nop]);
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
 * Check if user is in group
 * Returns 0 if valid, 1 otherwise.
 */
int is_user_in_group(char group[]) {
    __uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        perror(_("Error on getpwuid(): "));
    }

    int ngroups = 0;

    //this call is just to get the correct ngroups
    getgrouplist(pw->pw_name, pw->pw_gid, NULL, &ngroups);
    __gid_t groups[ngroups];

    //here we actually get the groups
    getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);


    //example to print the groups name
    int i;

    for (i = 0; i < ngroups; i++) {
        struct group *gr = getgrgid(groups[i]);

        if (gr == NULL) {
            perror(_("Error on getgrgid(): "));
        }
        if (strcmp(group, gr->gr_name) == 0) {
            printf(_("User is a valid member of project administrator " \
                     "group %s\n"), gr->gr_name);
            return 0;
        }
    }
    return 1;
}

/*
 * Returns true if path is under one of the given projects_roots. If true,
 * projects_root string is set with the matching projects_roots.
 *
 * The projects_root argument must be a preallocated string.
 *
 * Examples:
 *
 *   With:
 *
 *     projects_roots = { '/projects', '/data' }
 *     path = '/projects/awesome/data'
 *
 *     → is_in_projects_roots() returs true and set projects_root to '/projects'.
 *
 *   With:
 *
 *     projects_roots = { '/projects', '/data' }
 *     path = '/tmp/file'
 *
 *     → is_in_projects_roots() returns false (projects_root is not modified).
 */

bool is_in_projects_roots(char **projects_roots, char *projects_root,
                          const char *path) {

    for (int i = 0; i < nop; i++) {
        int l = strlen(projects_roots[i]);

        //if file in list of projects but not equal the project
        if ((strncmp(path, projects_roots[i], l) == 0)
            && (strcmp(path, projects_roots[i]))) {
            strncpy(projects_root, projects_roots[i], PATH_MAX);
            return true;
        }
    }

    return false;
}

/*
 * Set project_basedir and linux_group with respectively the name of the
 * of the base project directory and the name of the group of this directory.
 *
 * Example:
 *
 *    With:
 *
 *      projects_root = '/projects'
 *      path = '/projects/awesome/path/to/file'
 *      /projects/awesome directory belonging to root:physic
 *
 *      → project_admin_group() set
 *        - project_basedir to '/projects/awesome'
 *        - linux_group to 'physic'
 */

void project_admin_group(char *projects_root, char *project_basedir,
                         char *path, char *linux_group) {

    struct group *g;
    struct stat sb;

    // get the path of project
    int l = strlen(projects_root);
    int h = l + 1;

    /* clean allocated memory for strings */
    memset(project_basedir, 0, PATH_MAX);

    while (path[h] != '\0' && path[h] != '/') {
        h++;
    }
    strcpy(project_basedir, projects_root);
    /* concat with the basename of project directory */
    strncat(project_basedir, &path[l], h - l);

    VERBOSE(_("Project path: %s\n"), project_basedir);

    if (stat(project_basedir, &sb) == -1) {
        perror(_("Error on stat(): "));
        exit(EXIT_FAILURE);
    }

    VERBOSE(_("Project administrator group GID: %ld\n"), (long) sb.st_gid);

    g = getgrgid((long) sb.st_gid);
    strcpy(linux_group, g->gr_name);

    VERBOSE(_("Project administrator group name: %s\n"), linux_group);
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
    //set rwx to user and rw to group if its not a symlink
    struct stat buf;

    lstat(path, &buf);
    if (!S_ISLNK(buf.st_mode)) {
        struct stat st;

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
    char *projectsroot[PATH_MAX];
    char real_dir[PATH_MAX];
    bool isInProjectPath;
    char projectroot[PATH_MAX], projectdir[PATH_MAX], linux_group[PATH_MAX];
    struct stat path_stat;


    /* clean allocated memory for strings */
    memset(real_dir, 0, PATH_MAX);
    memset(projectroot, 0, PATH_MAX);
    memset(projectdir, 0, PATH_MAX);
    memset(linux_group, 0, PATH_MAX);

    read_config_file("/etc/prown.conf", projectsroot);

    // check the real path is correct
    if (!realpath(path, real_dir)) {
        ERROR(_("Path '%s' has not been found, it is discarded\n"), path);
        return 0;
    }

    /* check path is under projects roots directories */
    isInProjectPath =
        is_in_projects_roots(projectsroot, projectroot, real_dir);

    if (!isInProjectPath) {
        ERROR(_("Changing owner of file outside project parent directories "
                "is prohibited, path '%s' is discarded\n"), path);
        return 0;
    }

    /* get group owner of project base directory */
    project_admin_group(projectroot, projectdir, real_dir, linux_group);

    // if the user hasn't access to the project
    if (is_user_in_group(linux_group) == 1) {
        ERROR(_("Permission denied for project %s, you are not a member of "
                "this project administor group\n"), projectdir);
        return 0;
    }

    printf(_("Changing owner of directory %s\n"), real_dir);

    stat(real_dir, &path_stat);
    //if it's a file we should call setOwner one time
    if (path_stat.st_mode & S_IFREG) {
        printf(_("Changing owner of file %s\n"), real_dir);
        setOwner(real_dir);
    } else {
        // chown the real_dir if it's not the projectDir
        // because projectOwner() doesn't chown the entry path
        // but only the chlids
        if (strcmp(real_dir, projectdir))
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
                 "The user must be a member of project administrator group "
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
