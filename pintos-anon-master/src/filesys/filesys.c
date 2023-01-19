#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"

#define PATH_LENGTH 1<<8+1
#include "filesys/filesys.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "threads/thread.h"
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}
static void do_format(void)
{
    printf("Formatting file system...");
    free_map_create();
    if (!dir_create(ROOT_DIR_SECTOR, 16))
        PANIC("root directory creation failed");

    struct dir* direc = dir_open(inode_open(ROOT_DIR_SECTOR));
    dir_add(direc, ".", ROOT_DIR_SECTOR);
    dir_add(direc, "..", ROOT_DIR_SECTOR);
    dir_close(direc);

    free_map_close();
    printf("done.\n");
}
struct dir *get_path(char *name, char *parsed)
{
    //ASSERT(name);
   //ASSERT(parsed);
   //ASSERT(strlen(name));
    if (!name || !parsed || strlen(name) == 0)
        return NULL;
    char* tok1, * tok2, * dummy, * p;
    p = (char*)malloc(PATH_LENGTH);
    struct dir* direc = NULL;
    struct inode* i = NULL;
    strlcpy(p, name, PATH_LENGTH);

    if (p[0] != '/')	//directory from current directory
        direc = dir_reopen(thread_current()->direc);
    else	//directory from root
        direc = dir_open_root();
    if (!direc || !is_direc(dir_get_inode(direc))) return NULL;

    tok1 = strtok_r(p, "/", &dummy);
    tok2 = strtok_r(NULL, "/", &dummy);

    while (tok1 && tok2) {
        i = NULL;
        if (!dir_lookup(direc, tok1, &i) || !is_direc(i)) {
            dir_close(direc);
            return NULL;
        }
        else {
            dir_close(direc);
            direc = dir_open(i);
            tok1 = tok2;
            tok2 = strtok_r(NULL, "/", &dummy);
        }
    }
    if (tok1) strlcpy(parsed, tok1, PATH_LENGTH);
    else strlcpy(parsed, ".", PATH_LENGTH);
    free(p);
    return direc;
}
bool filesys_create_dir(char *name)
{
    char* parsed = (char*)malloc(PATH_LENGTH);
    struct dir* direc = get_path(name, parsed);
    block_sector_t sec = 0;
    bool flag = (direc && free_map_allocate(1, &sec) && dir_create(sec, 16) && dir_add(direc, parsed, sec));

    if (!flag) {
        if (sec) free_map_release(sec, 1);
        free(parsed);
        dir_close(direc);
        return false;
    }
    else {
        struct dir* tmp = dir_open(inode_open(sec));
        block_sector_t p = inode_get_inumber(dir_get_inode(direc));
        dir_add(tmp, ".", sec);
        dir_add(tmp, "..", p);

        dir_close(tmp);
        free(parsed);
        dir_close(direc);
        return true;
    }
    return false;
}

bool filesys_change_dir(char *path)
{
    char* temp = (char*)malloc(PATH_LENGTH);
    char* target = (char*)malloc(PATH_LENGTH);
    strlcpy(temp, path, PATH_LENGTH);
    strlcat(temp, "/0", PATH_LENGTH);
    struct dir* direc = get_path(temp, target);

    free(temp); free(target);
    if (!direc) return false;

    dir_close(thread_current()->direc);
    thread_current()->direc = direc;
    return true;
}
/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  block_sector_t inode_sector = 0;
  struct dir *dir = dir_open_root ();
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
