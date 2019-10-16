#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <dirent.h>

/*
 * Check if user has access to project 'path' .
 *
 * Returns 0 if valid, 1 otherwise.
 * */
int projectAccess(const char *path)
{
	int result = access(path, W_OK);
	return result;
}

/*
 * set recursively the user as the owner of the project.
 *
 * Returns 0 if valid, 1 otherwise.
 * */
int projectOwner(char *basepath){
    	char path[1000];
    	struct dirent *dp;
    	DIR *dir = opendir(basepath);
        int status = 0;
    	// Unable to open directory stream
    	if (!dir)
        	return 1;

    	while ((dp = readdir(dir)) != NULL)
    	{
    		if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0 && strcmp(dp->d_name, basepath) != 0)
        	{
        		// Construct new path from our base path
        		strcpy(path, basepath);
        		strcat(path, "/");
        		strcat(path, dp->d_name);
        		projectOwner(path);	
			//set user as the owner of the current file or directory
                        if (chown(path,getuid(),(gid_t)-1) != 0)
	                {
        	                perror("chown");
                	        exit(EXIT_FAILURE);
                	}
			//set rwx to user and rw to group 
                        if (chmod(path,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP) != 0)
			{
				perror("chmod");
				exit(EXIT_FAILURE);
			}
		}
	}
        closedir(dir);
	return status;
}


void read_str_from_config_line(char* config_line, char* val) {    
    	char prm_name[100];
    	sscanf(config_line, "%s %s\n", prm_name, val);
}


void read_config_file(char* config_filename, char* projectsdir) {
	FILE *fp;
    	char buf[50];
	if ((fp=fopen(config_filename, "r")) == NULL) {
        	fprintf(stderr, "Failed to open config file %s \n", config_filename);
        	exit(EXIT_FAILURE);
    	}
    	while(! feof(fp)) {
        	fgets(buf, 100, fp);
        	if (buf[0] == '#' || strlen(buf) < 4) {
            		continue;
        	}	
        	if (strstr(buf, "PROJECTS_DIR ")) {
            		read_str_from_config_line(buf, projectsdir);
        	}
    	}
    	fclose(fp);
}

int main(int argc, char **argv) {
        uid_t uid=getuid();
        char uid_str[50];
        char projectPath[argc+1][50]; /* List of project paths*/
        int validargs=0,i,j,nbarg,status;
        char projectroot[PATH_MAX], real_dir[argc][PATH_MAX];
        size_t lenprojectroot;
        char command[1000]="";
        
	read_config_file("/etc/prown.conf", projectroot); 
	sprintf(uid_str, "%d", uid);
        lenprojectroot=strlen(projectroot);
        if (argc == 1)
        {
                perror("ownProject");
                exit(EXIT_FAILURE);
        }
        else
        {
                j=0;
		//for each project
                for (i = 1; i < argc; i++)
                {
			// if the real path is correct
                        if (realpath(argv[i], real_dir[j]) != '\0')
                        {
				//calculate the real path lengh of the project
				lenprojectroot=strlen(projectroot);
                                // if the user hasn't access to the project 
                                if (projectAccess(real_dir[j]) != 0) {
                                        printf("Error: permission denied for project '%s' \n", real_dir[j]);
                                }
				// if the user passed path is in the ptoject path 
                                else if ((strstr(real_dir[j], projectroot) != NULL) && (strcmp(real_dir[j],projectroot)))
                                {
					
                                        printf("Setting owner of %s  directory %s to %d\n", argv[i], real_dir[j], uid,lenprojectroot);
                                        strcpy(projectPath[j],real_dir[j]);
                                        validargs=1;
                                        j++;
                                }
				//The else case is  when the passed project is not in projects path
                                else
                                {
                                        printf("You can't take rights everywhere. Directory '%s' will be ignored\n", argv[i]);
                                }
                        }
                        else
                        {
                                printf("Warning: directory '%s' not found! (ignored)\n", argv[i]);
                        }
                }
        }
        if (validargs == 1 )
        {
                for (i = 0; i < argc-1; i++)
                {
			status=projectOwner(projectPath[i]);
                }
        }
        else
        {
                printf("Nothing to do. Exiting...\n");
        }
}

