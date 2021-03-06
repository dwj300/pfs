#define PFS_BLOCK_SIZE 1 // 1 Kilobyte
#define STRIP_SIZE 4     // 4 blocks
#define NUM_FILE_SERVERS 2 //5
#define CLIENT_CACHE_SIZE 2 // 2 Megabytes
#define MAX_FILES 1000
#define MAX_CLIENTS 20

typedef struct pfs_stat {
  time_t pst_mtime; /* time of last data modification */
  time_t pst_ctime; /* time of creation */
  off_t pst_size;    /* File size in bytes */
} pfs_stat_t;
