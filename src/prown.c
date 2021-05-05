/*
 * Prown is a simple tool developed to give users the possibility to 
 * own projects (files and repositories).
 * Copyright (C) 2019 EDF SA.

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
#include "error.h"
#include <getopt.h>
#include <grp.h>


#define MAXLINE  1000

/* static variable for verbose mode */ 
static int verbose;
/* number of project paths in config file */ 
static int nop=0;

/*set user as the owner of the current file or directory*/
void setOwner(const char *path)
{	
	//use lchown to change owner for symlinks 
	if(verbose == 1){
		printf("changing owner of path %s\n",path);
	}
	if (lchown(path,getuid(),(gid_t)-1) != 0){
		perror("chown");
		exit(EXIT_FAILURE);
	}
	//set rwx to user and rw to group if its not a symlink
	struct stat buf;
        lstat(path, &buf);
	if (!S_ISLNK(buf.st_mode)){
		struct stat st;
		stat(path, &st);
		if (chmod(path,S_IRGRP|S_IWGRP|st.st_mode) != 0){
			perror("chmod");
			exit(EXIT_FAILURE);
		}
	}
}

/*
 * set recursively the user as the owner of the project.
 *
 * Returns 0 if valid, 1 otherwise.
 * */
int projectOwner(char *basepath){
	struct stat buf;
	lstat(basepath, &buf);
	int status = 0;
	if (!S_ISLNK(buf.st_mode)){
		char path[PATH_MAX];
		struct dirent *dp;
		DIR *dir = opendir(basepath);
		// Unable to open directory stream
		if (!dir)
			return 1;
		while ((dp = readdir(dir)) != NULL){
			if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0 && strcmp(dp->d_name, basepath) != 0){
				// Construct new path from our base path
				if (strlcpy(path, basepath,sizeof(path)) >= sizeof(path))
					exit(1);
				strcat(path, "/");
				strcat(path, dp->d_name);
				projectOwner(path);
				setOwner(path);
			}
		}
		closedir(dir);
	}
	return status;
}		

/*
 * Read lines from config file.
 * */
void read_str_from_config_line(char* config_line, char* val) {    
    	char prm_name[MAXLINE];
    	sscanf(config_line, "%s %s\n", prm_name, val);
}

/*
 * Read config file. projectsdir is the lis of projects
 * */
void read_config_file(char config_filename[], char* projectsdir[]) {
	FILE *fp;
    	char buf[MAXLINE];
	if ((fp=fopen(config_filename, "r")) == NULL) {
        	fprintf(stderr, "Failed to open config file %s \n", config_filename);
        	exit(EXIT_FAILURE);
    	}
    	while(! feof(fp)) {
        	fgets(buf, MAXLINE, fp);
        	if (buf[0] == '#' || strlen(buf) < 4) {
			continue;
        	}	
        	if (strstr(buf, "PROJECT_DIR ")) {
			if ((projectsdir[nop] = malloc(sizeof(char) * PATH_MAX)) == NULL) {
				printf("Unable to allocate memory \n");
				exit(1);
			}
			read_str_from_config_line(buf, projectsdir[nop]);
			nop++;
        	}
    	}
	fclose(fp);
}

/*
 * Check if user is in group
 * Returns 0 if valid, 1 otherwise.
 */
int is_user_in_group(char group[])
{	
	__uid_t uid = getuid();
	struct passwd* pw = getpwuid(uid);
	if(pw == NULL){
		perror("getpwuid error: ");
	}

	int ngroups = 0;

	//this call is just to get the correct ngroups
	getgrouplist(pw->pw_name, pw->pw_gid, NULL, &ngroups);
	__gid_t groups[ngroups];

	//here we actually get the groups
	getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);


	//example to print the groups name
        int i;
	for (i = 0; i < ngroups; i++){
		struct group* gr = getgrgid(groups[i]);
		if(gr == NULL){
			perror("getgrgid error: ");
		}
		if(strcmp(group,gr->gr_name)==0){
			printf("group of user: %s\n",gr->gr_name);
			return 0;
		}
	}
	return 1;
} 

void usage(int status) {
	if (status != EXIT_SUCCESS)
		printf("Saisissez « prown --help » pour plus d'informations.\n");
	else{
		printf("Utilisation: prown[OPTION]... FICHIER...\n");
		printf("Modifier le propriétaire du PROJET, FICHIER ou REPERTOIRE en PROPRIO actuel.\n");
		printf("\n-v, --verbose          afficher en détail les fichiers modifiés\n");
		printf("-h, --help             afficher l'aide et quitter\n");
		printf("\nL'utilisateur doit avoir le droit d'écriture sur le fichier ou le dossier\n");
		printf("qu'il souhaite posséder.\n");
		printf("\nExemples :\n");
		printf("  prown ccnhpc           devenir propriétaire sur le projet ccnhpc\n");
		printf("  prown ccnhpc saturne   devenir propriétaire sur le projet ccnhpc et saturne\n");
		printf("  prown ccnhpc/file      devenir propriétaire sur le fichier ccnhpc/file \n");
		
	}
}

int prownProject(char* path){
	uid_t uid=getuid();
	char projectPath[PATH_MAX]; /* List of project paths*/
	int validargs=0,i;
	char* projectsroot[PATH_MAX];
	char real_dir[PATH_MAX], projectroot[PATH_MAX], projectdir[PATH_MAX];
	char group[PATH_MAX],linux_group[PATH_MAX];
	struct stat sb;
	struct group *g;
	
	read_config_file("/etc/prown.conf", projectsroot);
	// if the real path is correct
	if (realpath(path, real_dir)){
		int isInProjectPath = 0;
		for (i=0; i<nop ; i++){
			int l=strlen(projectsroot[i]);
			//if file in list of projects but not equal the project
			if ((strncmp(real_dir, projectsroot[i],l)==0) && (strcmp(real_dir,projectsroot[i]))){
				strlcpy(projectroot,projectsroot[i],sizeof(projectroot));
				isInProjectPath = 1;
				// get the path of project
				int h=l+1;
				while (real_dir[h] !='\0' && real_dir[h] != '/'){
					h++;
				}
				memcpy(group, &real_dir[l],h-l);
				strcpy(projectdir, projectroot);
				strcat(projectdir, group);
				if(verbose == 1){
					printf("Project path: %s\n",projectdir);
				}
				if (stat(projectdir, &sb) == -1) {
					perror("stat");
					exit(EXIT_FAILURE);
				}
				if(verbose == 1){
					printf("Ownership: GID=%ld\n",(long) sb.st_gid);
				}
				g = getgrgid((long) sb.st_gid);
				strcpy(linux_group, g->gr_name);
				if(verbose == 1){
					printf("%s\n", linux_group);
				}
				break;
			}
		}
		// if the user hasn't access to the project 
		if (is_user_in_group(linux_group)==1 && isInProjectPath == 1) {	
			printf("Error: permission denied for project \n");
			printf("       you are not in '%s' group \n", linux_group);
		}
		// if the user passed path is in the ptoject path 
		else if (isInProjectPath == 1){
			printf("Setting owner of %s  directory %s to %d\n", path, real_dir, uid);
			if (strlcpy(projectPath,real_dir,sizeof(projectPath)) >= sizeof(projectPath))
				exit(1);
			struct stat path_stat;
			stat(projectPath, &path_stat);
			//if it's a file we should call setOwner one time
			if (path_stat.st_mode & S_IFREG)			{
				printf("owning file %s\n", projectPath);
				setOwner(projectPath);
			}
			else{
				//chown the projectPath if it's not the projectDir
				// because projectOwner() doesn't chown the entry path
				// but only the chlids
				if (strcmp(projectPath,projectdir))
					setOwner(projectPath);
				projectOwner(projectPath);
			}
			validargs=1;
		}
		//The else case is  when the passed project is not in projects path
		else{
			printf("You can't take rights everywhere. Directory '%s' will be ignored\n", path);
		}
	}
	else{
		printf("Warning: directory '%s' not found! (ignored)\n", path);
	}

	return validargs;
}

int main(int argc, char **argv) {
	char *options = "hv";
	int longindex;
	int opt;
	int help=0;	
	struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"verbose", no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};
	
	while ((opt = getopt_long(argc, argv, options, longopts, &longindex)) != -1) {
		switch (opt) {
		case 'v':
		   verbose++;
		   break;
		case 'h':
		   help=1;
		   usage(EXIT_SUCCESS);
		   break;
		default:
		   usage(EXIT_FAILURE);
		   break;
		}
	}
	if ((argc == 1 || optind == argc)&&(help != 1)){
		error(0, 0, "opérande manquant");
		usage(EXIT_FAILURE);
	}
	else{
		for(; optind < argc; optind++){
			char *path = argv[optind];
			prownProject(path);
		}
	}
}

