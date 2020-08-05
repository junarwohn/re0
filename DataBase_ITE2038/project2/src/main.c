#include "bpt.h"
//#include "../include/bpt.h"
#include "file.h"

// Get global variable of file pointer of datafile.
extern FILE * fp_db;

// MAIN

int main( int argc, char ** argv ) {

    char * input_file;
    FILE * fp;
    node * root;
    int input, range2;
    char instruction;
    char license_part;
    char input_val[120];

    root = NULL;
    verbose_output = false;

    if (argc > 1) {
        order = atoi(argv[1]);
        if (order < MIN_ORDER || order > MAX_ORDER) {
            fprintf(stderr, "Invalid order: %d .\n\n", order);
            usage_3();
            exit(EXIT_FAILURE);
        }
    }

    license_notice();
    usage_1();  
    usage_2();

    if (argc > 2) {
        if (open_table(argv[2]) == -1) {
            fprintf(stderr, "Cannot load file %s \n\n", argv[2]);
            exit(EXIT_FAILURE);
        }
        while (!feof(fp)) {
            fscanf(fp, "%d %s\n", &input, input_val);
            if(db_insert(input, input_val) != 0) {
                fprintf(stderr, "Cannot write file %s \n\n", argv[2]);
                exit(EXIT_FAILURE);
            }
        }
        fclose(fp);
        print_tree(root);
    }

    /*  
    if (argc > 2) {
        input_file = argv[2];
        fp = fopen(input_file, "r");
        if (fp == NULL) {
            perror("Failure  open input file.");
            exit(EXIT_FAILURE);
        }
        while (!feof(fp)) {
            fscanf(fp, "%d\n", &input);
            root = insert(root, input, input);
        }
        fclose(fp);
        print_tree(root);
    }
    */

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            scanf("%d", &input);
            root = delete(root, input);
            print_tree(root);
            break;
        case 'i':
            scanf("%d %s", &input, input_val);
            if(db_insert(input, input_val) != 0) {
                fprintf(stderr, "Cannot write input %d %s \n\n", input, input_val);
                exit(EXIT_FAILURE);
            }
            print_tree(root);
            break;
        case 'f':
        case 'p':
            scanf("%d", &input);
            find_and_print(root, input, instruction == 'p');
            break;
        case 'r':
            scanf("%d %d", &input, &range2);
            if (input > range2) {
                int tmp = range2;
                range2 = input;
                input = tmp;
            }
            find_and_print_range(root, input, range2, instruction == 'p');
            break;
        case 'l':
            print_leaves(root);
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        case 't':
            print_tree(root);
            break;
        case 'v':
            verbose_output = !verbose_output;
            break;
        case 'x':
            if (root)
                root = destroy_tree(root);
            print_tree(root);
            break;
        default:
            usage_2();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");

    return EXIT_SUCCESS;
}
