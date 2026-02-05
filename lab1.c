
//Template 1: “Open → Read → Close”

DIR *dirp;
struct dirent *dp;

if ((dirp = opendir(PATH)) == NULL)
    ERR("opendir");

errno = 0;
while ((dp = readdir(dirp)) != NULL)
{
   //do something
}

if (errno != 0)
    ERR("readdir");

if (closedir(dirp))
    ERR("closedir");


//Template 2: “Directory entry → file metadata”
//Whenever they want file type / size / permissions

struct stat st;

if (lstat(dp->d_name, &st))
    ERR("lstat");

// now you can use st.st_mode, st.st_size, etc.



// Template 3: File type decision block

if (S_ISDIR(st.st_mode))
{
}
else if (S_ISREG(st.st_mode))
{
}
else if (S_ISLNK(st.st_mode))
{
}
else
{
}




//Template 4: Ignore special entries
//We skip current and parent directory to avoid loops.

if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
    continue;





//Template 5: Safe path building
//

char path[1024];

snprintf(path, sizeof(path), "%s/%s", base, dp->d_name);





//Template 6: Minimal full frame

void process_dir(const char *path)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat st;
    char fullpath[1024];

    if ((dirp = opendir(path)) == NULL)
        ERR("opendir");

    errno = 0;
    while ((dp = readdir(dirp)) != NULL)
    {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, dp->d_name);

        if (lstat(fullpath, &st))
            ERR("lstat");

        // TASK LOGIC GOES HERE
    }

    if (errno)
        ERR("readdir");

    if (closedir(dirp))
        ERR("closedir");
}


//Template 7: Recursive directory (but it is better to use ntfw)

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void walk_dir(const char *path)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat st;
    char fullpath[1024];

    if ((dirp = opendir(path)) == NULL)
        ERR("opendir");

    errno = 0;
    while ((dp = readdir(dirp)) != NULL)
    {
        // skip special entries
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            continue;

        // build full path 
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, dp->d_name);

        // get metadata
        if (lstat(fullpath, &st))
            ERR("lstat");

        //TASK LOGIC GOES HERE

        // recursive descent
        if (S_ISDIR(st.st_mode))
            walk_dir(fullpath);
    }

    if (errno)
        ERR("readdir");

    if (closedir(dirp))
        ERR("closedir");
}

int main(int argc, char **argv)
{
    walk_dir(".");
    return EXIT_SUCCESS;
}




//Template 8: Choose directory
//was written in main
char path[MAX_PATH];
//save current directory to path
    if (getcwd(path, MAX_PATH) == NULL)
        ERR("getcwd");
    for (int i = 1; i < argc; i++)
    { //change to provided directory
        if (chdir(argv[i]))
            ERR("chdir");
        printf("%s:\n", argv[i]);
        scan_dir();
        //go back to initial directory
        if (chdir(path))
            ERR("chdir");
    }