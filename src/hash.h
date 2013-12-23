/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 */

/*
 * The number of buckets for the hash table contained in each list.  This
 * should probably be prime.
 */
#define HASHSIZE	151

/*
 * Types of nodes
 */
enum ntype
{
    NT_UNKNOWN, HEADER, ENTRIES, FILES, LIST, RCSNODE,
    RCSVERS, DIRS, UPDATE, LOCK, NDBMNODE, FILEATTR,
    VARIABLE, RCSFIELD, RCSCMPFLD
};
typedef enum ntype Ntype;

struct node
{
    Ntype type;
    struct node *next;
    struct node *prev;
    struct node *hashnext;
    struct node *hashprev;
    char *key;
    char *data;
    void (*delproc) (struct node *);
};
typedef struct node Node;

struct list_t
{
    Node *list;
    Node *hasharray[HASHSIZE];
    struct list_t *next;
};
typedef struct list_t List;

List *getlist();
Node *findnode();
Node *findnode_fn();
Node *getnode();
int insert_before();
int addnode();
int addnode_at_front();
int walklist (List *list, int (*proc)(Node *, void *), void *closure);
int list_isempty();
void dellist();
void delnode();
void freenode();
int freenodecache();
int freelistcache();
void sortlist();
int fsortcmp();
