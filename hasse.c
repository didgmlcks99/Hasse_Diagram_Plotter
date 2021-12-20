#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

struct Node{
    struct Node *lower;
    char name[16];
    struct Node *upper;
};

void check_input(int argc, char **args);
FILE *open_file(char *filename);
void draw_hasse(FILE *fp);
int accumulate_datas(FILE *fp, char ***datas);
int initial_nodes(char*** datas, int size, struct Node *nodes);
void link_nodes(char*** datas, int datas_cnt, struct Node *nodes, int nodes_cnt);
void print_ds(char ***datas, int size);
void print_nd(struct Node *nodes, int size);
char *trim(char *str);

int main(int argc, char **args)
{
    char *filename = args[1];
    FILE *fp = open_file(filename);
    draw_hasse(fp);
    fclose(fp);
    return 0;
}

void check_input(int argc, char **args)
{
    printf("argc: %d\n", argc);

    for (int i = 0; i < argc; i++)
    {
        printf("%d. %s\n", i + 1, args[i]);
    }
}

FILE *open_file(char *filename)
{
    FILE *fp;

    fp = fopen(filename, "r");

    if (fp == NULL)
    {
        printf("[FILE ERROR] Cannot open file \n");
        exit(0);
    }

    return fp;
}

void draw_hasse(FILE *fp){

    char ***datas = (char ***)malloc(32 * sizeof(char **));
    for(int i=0; i<32; i++){
        datas[i] = (char **)malloc(2 * sizeof(char *));
        for(int j=0; j<2; j++){
            datas[i][j] = (char *)malloc(16 * sizeof(char));
        }
    }

    // accumulate input datas
    int datas_cnt = accumulate_datas(fp, datas);
    print_ds(datas, datas_cnt);

    printf("\n");

    // create distinct nodes and link its adjacent nodes from datas
    struct Node *nodes = (struct Node*)malloc(datas_cnt * sizeof(struct Node));
    int nodes_cnt = initial_nodes(datas, datas_cnt, nodes);
    link_nodes(datas, datas_cnt, nodes, nodes_cnt);

    print_nd(nodes, nodes_cnt);
}

int accumulate_datas(FILE *fp, char ***datas){
    int datas_cnt = 0;

    char line[64];
    while(fgets(line, sizeof(line), fp) != NULL){
        char *token = strtok(line, "_");
        strcpy(datas[datas_cnt][0], token);

        token = strtok(NULL, " ");
        trim(token);
        strcpy(datas[datas_cnt][1], token);

        datas_cnt++;
    }

    return datas_cnt;
}

int initial_nodes(char*** datas, int size, struct Node *nodes){
    int nodes_cnt = 0;
    int flag = 0;

    for(int i=0; i<size; i++){
        flag = 0;
        for(int j=0; j<nodes_cnt; j++){
            if(strcmp(datas[i][0], nodes[j].name) == 0){
                flag = 1;
                break;
            }
        }

        if(flag == 0){
            strcpy(nodes[nodes_cnt].name, datas[i][0]);
            nodes[nodes_cnt].lower = 0x0;
            nodes[nodes_cnt].upper = 0x0;
            nodes_cnt++;
        }

        flag = 0;
        for(int j=0; j<nodes_cnt; j++){
            if(strcmp(datas[i][1], nodes[j].name) == 0){
                flag = 1;
                break;
            }
        }

        if(flag == 0){
            strcpy(nodes[nodes_cnt].name, datas[i][1]);
            nodes[nodes_cnt].lower = 0x0;
            nodes[nodes_cnt].upper = 0x0;
            nodes_cnt++;
        }
    }

    return nodes_cnt;
}

void link_nodes(char*** datas, int datas_cnt, struct Node *nodes, int nodes_cnt){
    for(int i=0; i<datas_cnt; i++){
        if(strcmp(datas[i][0], datas[i][1]) != 0){
            for(int j=0; j<nodes_cnt; j++){
                if(strcmp(nodes[j].name, datas[i][0]) == 0){
                    struct Node *node_ptr = &nodes[j];
                    while(node_ptr->upper != 0x0){
                        node_ptr = node_ptr->upper;
                    }

                    struct Node *new_node = (struct Node*)malloc(sizeof(struct Node));
                    strcpy(new_node->name, datas[i][1]);
                    new_node->lower = 0x0;
                    new_node->upper = 0x0;

                    node_ptr->upper = new_node;

                    break;
                }
            }
        }
    }
}

void print_ds(char ***datas, int size){
    for(int i=0; i<size; i++){
        printf("%s : %s\n", datas[i][0], datas[i][1]);
    }
}

void print_nd(struct Node *nodes, int size){
    for(int i=0; i<size; i++){
        printf("%s : ", nodes[i].name);

        struct Node *node_ptr = &nodes[i];
        while(node_ptr->upper != 0x0){
            node_ptr = node_ptr->upper;
            printf("%s, ", node_ptr->name);
        }
        
        printf("\n");
    }
}

// https://www.delftstack.com/ko/howto/c/trim-string-in-c/
char *trim(char *str)
{
    char *end;

    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)
        return str;

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    end[1] = '\0';

    return str;
}