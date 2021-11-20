
#define LINE_LENGTH 1000

typedef struct Line {
    int pid;
    char line[LINE_LENGTH];
} Line;


union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

union semun arg;

