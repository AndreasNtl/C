#include <stdio.h>
#include <stdlib.h>
#include "interfaces.h"

int main(int argc, char** argv) {
    int no_of_p;
    int count_c;
    int total_match = 0;

    if (argc == 1) {
        printf("Invalid parameters \n");
        return 1;
    } else if (argc == 3) {
        no_of_p = atoi(argv[1]);
        count_c = atoi(argv[2]);
        total_match = m(no_of_p, count_c);
        
        printf("number of p is %d \n", no_of_p);
        printf("count c is  %d \n", count_c);
        printf("sum of match is %d \n", total_match);
    } else {
        printf("Invalid parameters \n");
        return 1;
    }
    return 0;
}

