#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "ezdib.h"

struct Node
{
    char name[64];
    int level;
    int x, y;
    struct Node *children;
};

void check_input(int argc, char **args);
FILE *open_file(char *filename);
void draw_hasse(FILE *fp, char *filename);
void make_bmp(char *filename, struct Node *nodes, int nodes_cnt, int max_lvl, int max_children);
void assign_levels(struct Node *nodes, int nodes_cnt);
void traverse_nd(struct Node *nodes, struct Node node, int lvl, int nodes_cnt);
int accumulate_datas(FILE *fp, char ***datas);
int initial_nodes(char ***datas, int size, struct Node *nodes);
void link_nodes(char ***datas, int datas_cnt, struct Node *nodes, int nodes_cnt);
void print_ds(char ***datas, int size);
void print_nd(struct Node *nodes, int size);
char *trim(char *str);
void validate_input(struct Node *nodes, int nodes_cnt);
int get_max_lvl(struct Node *nodes, int nodes_cnt);
int get_max_children(int max_lvl, struct Node *nodes, int nodes_cnt);

int main(int argc, char **args)
{
    char *filename = args[1];
    FILE *fp = open_file(filename);
    draw_hasse(fp, filename);
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

void draw_hasse(FILE *fp, char *filename)
{
    char ***datas = (char ***)malloc(64 * sizeof(char **));
    for (int i = 0; i < 32; i++)
    {
        datas[i] = (char **)malloc(2 * sizeof(char *));
        for (int j = 0; j < 2; j++)
        {
            datas[i][j] = (char *)malloc(64 * sizeof(char));
        }
    }

    // accumulate input datas
    int datas_cnt = accumulate_datas(fp, datas);
    print_ds(datas, datas_cnt);

    printf("\n");

    /*
     * 1. create distinct nodes,
     * 2. link its adjacent nodes,
     * 3. assign according levels to each node,
     * using information from datas
     */
    struct Node *nodes = (struct Node *)malloc(datas_cnt * sizeof(struct Node));
    int nodes_cnt = initial_nodes(datas, datas_cnt, nodes);
    validate_input(nodes, nodes_cnt);
    link_nodes(datas, datas_cnt, nodes, nodes_cnt);
    assign_levels(nodes, nodes_cnt);
    int max_lvl = get_max_lvl(nodes, nodes_cnt);
    int max_children = get_max_children(max_lvl, nodes, nodes_cnt);
    print_nd(nodes, nodes_cnt);

    make_bmp(filename, nodes, nodes_cnt, max_lvl, max_children);
}

void make_bmp(char *filename, struct Node *nodes, int nodes_cnt, int max_lvl, int max_children)
{
    HEZDIMAGE hDib;
    HEZDFONT hFont;

    // Create output file name

    // Create image
    // x, y, ?, ?
    printf("max_children: %d\n", max_children);
    printf("max_lvl: %d\n", max_lvl);

    int x_size = (300 * max_children);
    int y_size = (max_lvl + 1) * 250;
    hDib = ezd_create(x_size, y_size * (-1), 32, 0);
    // hDib = ezd_create(1000, -1000, 32, 0);

    printf("x_size: %d\n", x_size);
    printf("y_size: %d\n", y_size);

    // Fill in the background
    ezd_fill(hDib, 0x404040);

    // Test fonts
    hFont = ezd_load_font(EZD_FONT_TYPE_MEDIUM, 0, 0);
    ezd_text(hDib, hFont, "ITP 20002-03 Discrete Mathematics", -1, 30, 30, 0x00ff00);
    ezd_text(hDib, hFont, "HW4. Hasse Diagram Plotter", -1, 30, 50, 0x00ff00);
    ezd_text(hDib, hFont, "21800436 YANG HEECHAN", -1, 30, 70, 0x00ff00);

    printf("\n");
    printf("********** (x, y) coordinate for each nodes **********\n");
    printf("----------------------------------------------------------\n");

    int diagram_index[nodes_cnt][nodes_cnt];

    // draw node circles in place
    int overall = 0;
    for (int i = 0; i < nodes_cnt; i++)
    {
        int index = 0;

        for (int j = 0; j < nodes_cnt; j++)
        {
            if (nodes[j].level == i)
            {
                diagram_index[i][index] = j;
                index++;
                overall++;
            }
        }

        for (int k = 0; k < index; k++)
        {
            int id = diagram_index[i][k];
            int lvl = nodes[id].level;

            nodes[id].y = (y_size - 100) - (lvl * 250);
            nodes[id].x = (x_size / (index + 1)) + (((x_size / index) * k) - 50) + (lvl % 2);

            int y = nodes[id].y;
            int x = nodes[id].x;

            ezd_circle(hDib, x, y, 50, 0x00ff00);
            ezd_text(hDib, hFont, nodes[id].name, -1, x - 25, y, 0x00ff00);
        }

        if (overall == nodes_cnt)
        {
            break;
        }
    }

    // draw lines
    for (int i = 0; i < nodes_cnt; i++)
    {
        struct Node *node_ptr = &nodes[i];

        int x1 = node_ptr->x;
        int y1 = node_ptr->y - 50;

        printf("> %s : (%d, %d)\n", node_ptr->name, x1, y1);

        int x2, y2;

        while (node_ptr->children != 0x0)
        {
            node_ptr = node_ptr->children;

            for (int j = 0; j < nodes_cnt; j++)
            {
                if (strcmp(node_ptr->name, nodes[j].name) == 0)
                {
                    x2 = nodes[j].x;
                    y2 = nodes[j].y + 50;
                    break;
                }
            }

            ezd_line(hDib, x1, y1, x2, y2, 0x00ff00);
        }
    }

    // Save the test image
    char *bmpname = (char *)malloc(16 * sizeof(char));
    strcpy(bmpname, filename);
    strcat(bmpname, ".bmp");
    ezd_save(hDib, bmpname);

    /// Releases the specified font
    if (hFont)
        ezd_destroy_font(hFont);

    // Free resources
    if (hDib)
        ezd_destroy(hDib);
}

void assign_levels(struct Node *nodes, int nodes_cnt)
{
    for (int i = 0; i < nodes_cnt; i++)
    {
        int init_lvl = 0;

        if (nodes[i].level == -1)
        {
            nodes[i].level = init_lvl;
        }
        else
        {
            init_lvl = nodes[i].level;
        }

        traverse_nd(nodes, nodes[i], init_lvl, nodes_cnt);
    }
}

void traverse_nd(struct Node *nodes, struct Node node, int lvl, int nodes_cnt)
{
    struct Node *node_ptr = &node;

    while (node_ptr->children != 0x0)
    {
        node_ptr = node_ptr->children;
        for (int i = 0; i < nodes_cnt; i++)
        {
            if (strcmp(nodes[i].name, node_ptr->name) == 0)
            {
                if (nodes[i].level <= lvl)
                {
                    nodes[i].level = lvl + 1;
                    node_ptr->level = lvl + 1;
                    traverse_nd(nodes, nodes[i], lvl + 1, nodes_cnt);
                }
                else
                {
                    node_ptr->level = nodes[i].level;
                }
                break;
            }
        }
    }
}

int accumulate_datas(FILE *fp, char ***datas)
{
    int datas_cnt = 0;

    char line[64];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        char *token = strtok(line, "_");
        strcpy(datas[datas_cnt][0], token);

        token = strtok(NULL, " ");
        trim(token);
        strcpy(datas[datas_cnt][1], token);

        datas_cnt++;
    }

    return datas_cnt;
}

int initial_nodes(char ***datas, int size, struct Node *nodes)
{
    int nodes_cnt = 0;
    int flag = 0;

    for (int i = 0; i < size; i++)
    {
        flag = 0;
        for (int j = 0; j < nodes_cnt; j++)
        {
            if (strcmp(datas[i][0], nodes[j].name) == 0)
            {
                flag = 1;
                break;
            }
        }

        if (flag == 0)
        {
            strcpy(nodes[nodes_cnt].name, datas[i][0]);
            nodes[nodes_cnt].children = 0x0;
            nodes[nodes_cnt].level = -1;
            nodes[nodes_cnt].x = -1;
            nodes[nodes_cnt].y = -1;
            nodes_cnt++;
        }

        flag = 0;
        for (int j = 0; j < nodes_cnt; j++)
        {
            if (strcmp(datas[i][1], nodes[j].name) == 0)
            {
                flag = 1;
                break;
            }
        }

        if (flag == 0)
        {
            strcpy(nodes[nodes_cnt].name, datas[i][1]);
            nodes[nodes_cnt].children = 0x0;
            nodes[nodes_cnt].level = -1;
            nodes[nodes_cnt].x = -1;
            nodes[nodes_cnt].y = -1;
            nodes_cnt++;
        }
    }

    return nodes_cnt;
}

void link_nodes(char ***datas, int datas_cnt, struct Node *nodes, int nodes_cnt)
{
    for (int i = 0; i < datas_cnt; i++)
    {
        if (strcmp(datas[i][0], datas[i][1]) != 0)
        {
            for (int j = 0; j < nodes_cnt; j++)
            {
                if (strcmp(nodes[j].name, datas[i][0]) == 0)
                {
                    struct Node *node_ptr = &nodes[j];
                    while (node_ptr->children != 0x0)
                    {
                        node_ptr = node_ptr->children;
                    }

                    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
                    strcpy(new_node->name, datas[i][1]);
                    new_node->children = 0x0;
                    nodes[nodes_cnt].level = -1;
                    nodes[nodes_cnt].x = -1;
                    nodes[nodes_cnt].y = -1;

                    node_ptr->children = new_node;

                    break;
                }
            }
        }
    }
}

void print_ds(char ***datas, int size)
{
    printf("********** input pairs **********\n");
    printf("----------------------------------------------------------\n");
    for (int i = 0; i < size; i++)
    {
        printf("%s : %s\n", datas[i][0], datas[i][1]);
    }
    printf("\n========================================================\n");
}

void print_nd(struct Node *nodes, int size)
{
    printf("********** adjacency list for each node with level (#) **********\n");
    printf("----------------------------------------------------------\n");
    for (int i = 0; i < size; i++)
    {
        printf("%s(%d) : ", nodes[i].name, nodes[i].level);

        struct Node *node_ptr = &nodes[i];
        while (node_ptr->children != 0x0)
        {
            node_ptr = node_ptr->children;
            printf("%s(%d), ", node_ptr->name, node_ptr->level);
        }

        printf("\n");
    }
    printf("\n========================================================\n");
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

void validate_input(struct Node *nodes, int nodes_cnt)
{
    if (nodes_cnt > 32)
    {
        printf("[INVALID INPUT] input data file contains information of more than 32 vertices.\n");
        printf("--> input data file contains %d vertices\n", nodes_cnt);
        exit(0);
    }

    for (int i = 0; i < nodes_cnt; i++)
    {
        int node_length = (int)strlen(nodes[i].name);
        if (node_length > 16)
        {
            printf("[INVALID INPUT] a node can have at most 16 alphanumeric characters.\n");
            printf("--> %s has a character length of %d\n", nodes[i].name, (int)strlen(nodes[i].name));
            exit(0);
        }
    }
}

int get_max_lvl(struct Node *nodes, int nodes_cnt)
{
    int max_lvl = -1;
    for (int i = 0; i < nodes_cnt; i++)
    {
        if (nodes[i].level > max_lvl)
            max_lvl = nodes[i].level;
    }
    return max_lvl;
}

int get_max_children(int max_lvl, struct Node *nodes, int nodes_cnt)
{
    int max_each_lvl[max_lvl];
    for (int i = 0; i < max_lvl; i++)
    {
        max_each_lvl[i] = 0;
    }

    for (int i = 0; i < nodes_cnt; i++)
    {
        int lvl = nodes[i].level;
        max_each_lvl[lvl]++;
    }

    int max_children = -1;
    for (int i = 0; i < max_lvl; i++)
    {
        if (max_each_lvl[i] > max_children)
            max_children = max_each_lvl[i];
    }

    return max_children;
}