#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "ezdib.h"

struct Node
{
    char name[16];
    int level;
    int x, y;
    struct Node *children;
};

void check_input(int argc, char **args);
FILE *open_file(char *filename);
void draw_hasse(FILE *fp, char *filename);
void draw_traverse(HEZDIMAGE hDib, HEZDFONT hFont, struct Node node, struct Node *nodes, int nodes_cnt, int x, int y);
void make_bmp(char *filename, struct Node *nodes, int nodes_cnt);
void assign_levels(struct Node *nodes, int nodes_cnt);
void traverse_nd(struct Node *nodes, struct Node node, int lvl, int nodes_cnt);
int accumulate_datas(FILE *fp, char ***datas);
int initial_nodes(char ***datas, int size, struct Node *nodes);
void link_nodes(char ***datas, int datas_cnt, struct Node *nodes, int nodes_cnt);
void print_ds(char ***datas, int size);
void print_nd(struct Node *nodes, int size);
char *trim(char *str);

int bar_graph(HEZDIMAGE x_hDib, HEZDFONT x_hFont, int x1, int y1, int x2, int y2,
              int nDataType, void *pData, int nDataSize, int *pCols, int nCols)
{
    int i, c, w, h;
    int tyw = 0, bw = 0;
    double v, dMin, dMax, dRMin, dRMax;

    // Sanity checks
    if (!pData || 0 >= nDataSize || !pCols || !nCols)
        return 0;

    // Get the range of the data set
    ezd_calc_range(nDataType, pData, nDataSize, &dMin, &dMax, 0);

    // Add margin to range
    dRMin = dMin - (dMax - dMin) / 10;
    dRMax = dMax + (dMax - dMin) / 10;

    if (x_hFont)
    {
        char num[256] = {0};

        // Calculate text width of smallest value
        sprintf(num, "%.2f", dMin);
        ezd_text_size(x_hFont, num, -1, &tyw, &h);
        ezd_text(x_hDib, x_hFont, num, -1, x1, y2 - (h * 2), *pCols);

        // Calculate text width of largest value
        sprintf(num, "%.2f", dMax);
        ezd_text_size(x_hFont, num, -1, &w, &h);
        ezd_text(x_hDib, x_hFont, num, -1, x1, y1 + h, *pCols);
        if (w > tyw)
            tyw = w;

        // Text width margin
        tyw += 10;

    } // end if

    // Draw margins
    ezd_line(x_hDib, x1 + tyw - 2, y1, x1 + tyw - 2, y2, *pCols);
    ezd_line(x_hDib, x1 + tyw - 2, y2, x2, y2, *pCols);

    // Calculate bar width
    bw = (x2 - x1 - tyw - nDataSize * 2) / nDataSize;

    // Draw the bars
    c = 0;
    for (i = 0; i < nDataSize; i++)
    {
        if (++c >= nCols)
            c = 1;

        // Get the value for this element
        v = ezd_scale_value(i, nDataType, pData, dRMin, dRMax - dRMin, 0, y2 - y1 - 2);

        // Fill in the bar
        ezd_fill_rect(x_hDib, x1 + tyw + i + ((bw + 1) * i), y2 - (int)v - 2,
                      x1 + tyw + i + ((bw + 1) * i) + bw, y2 - 2, pCols[c]);

        // Outline the bar
        ezd_rect(x_hDib, x1 + tyw + i + ((bw + 1) * i), y2 - (int)v - 2,
                 x1 + tyw + i + ((bw + 1) * i) + bw, y2 - 2, *pCols);
    } // end for

    return 1;
}

#define PI ((double)3.141592654)
#define PI2 ((double)2 * PI)

int pie_graph(HEZDIMAGE x_hDib, int x, int y, int rad,
              int nDataType, void *pData, int nDataSize, int *pCols, int nCols)
{
    int i, c;
    double v, pos, dMin, dMax, dTotal;

    // Sanity checks
    if (!pData || 0 >= nDataSize || !pCols || !nCols)
        return 0;

    // Draw chart outline
    ezd_circle(x_hDib, x, y, rad, *pCols);

    // Get the range of the data set
    ezd_calc_range(nDataType, pData, nDataSize, &dMin, &dMax, &dTotal);

    // Draw the pie slices
    pos = 0;
    c = 0;
    ezd_line(x_hDib, x, y, x + rad, y, *pCols);
    for (i = 0; i < nDataSize; i++)
    {
        if (++c >= nCols)
            c = 1;

        // Get the value for this element
        v = ezd_scale_value(i, nDataType, pData, 0, dTotal, 0, PI2);

        ezd_line(x_hDib, x, y,
                 x + (int)((double)rad * cos(pos + v)),
                 y + (int)((double)rad * sin(pos + v)),
                 *pCols);

        ezd_flood_fill(x_hDib, x + (int)((double)rad / (double)2 * cos(pos + v / 2)),
                       y + (int)((double)rad / (double)2 * sin(pos + v / 2)),
                       *pCols, pCols[c]);

        pos += v;

    } // end for

    return 1;
}

typedef struct _SAsciiData
{
    int sw;
    unsigned char *buf;
} SAsciiData;

int ascii_writer(void *pUser, int x, int y, int c, int f)
{
    SAsciiData *p = (SAsciiData *)pUser;
    unsigned char ch = (unsigned char)(f & 0xff);

    if (!p)
        return 0;

    if (('0' <= ch && '9' >= ch) || ('A' <= ch && 'Z' >= ch) || ('a' <= ch && 'z' >= ch))

        // Write the character
        p->buf[y * p->sw + x] = (unsigned char)f;

    else

        // Write the color
        p->buf[y * p->sw + x] = (unsigned char)c;

    return 1;
}

typedef struct _SDotMatrixData
{
    int w;
    int h;
    HEZDIMAGE pDib;
} SDotMatrixData;

int dotmatrix_writer(void *pUser, int x, int y, int c, int f)
{
    int cc, r, dw = 3;
    HEZDIMAGE hDib = (HEZDIMAGE)pUser;

    if (!hDib)
        return 0;

    cc = c & 0xff;
    for (r = 0; r < dw; r++)
    {
        ezd_circle(hDib, x * dw * 2, y * dw * 2, r, cc);
        if (r)
            cc >>= 1;
    } // end for

    cc = (c >> 8) & 0xff;
    for (r = 0; r < dw; r++)
    {
        ezd_circle(hDib, x * dw * 2 + dw, y * dw * 2, r, cc << 8);
        if (r)
            cc >>= 1;
    } // end for

    cc = c & 0xff;
    for (r = 0; r < dw; r++)
    {
        ezd_circle(hDib, x * dw * 2 + dw, y * dw * 2 + dw, r, cc);
        if (r)
            cc >>= 1;
    } // end for

    cc = (c >> 16) & 0xff;
    for (r = 0; r < dw; r++)
    {
        ezd_circle(hDib, x * dw * 2, y * dw * 2 + dw, r, cc << 16);
        if (r)
            cc >>= 1;
    } // end for

    return 1;
}

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
    char ***datas = (char ***)malloc(32 * sizeof(char **));
    for (int i = 0; i < 32; i++)
    {
        datas[i] = (char **)malloc(2 * sizeof(char *));
        for (int j = 0; j < 2; j++)
        {
            datas[i][j] = (char *)malloc(16 * sizeof(char));
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
    link_nodes(datas, datas_cnt, nodes, nodes_cnt);
    assign_levels(nodes, nodes_cnt);
    print_nd(nodes, nodes_cnt);

    make_bmp(filename, nodes, nodes_cnt);
}

void make_bmp(char *filename, struct Node *nodes, int nodes_cnt)
{
    HEZDIMAGE hDib;
    HEZDFONT hFont;

    // Create output file name

    // Create image
    // x, y, ?, ?
    hDib = ezd_create(1500, -1000, 32, 0);

    // Fill in the background
    ezd_fill(hDib, 0x404040);

    // Test fonts
    hFont = ezd_load_font(EZD_FONT_TYPE_MEDIUM, 0, 0);

    printf("\n");

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

            nodes[id].y = 700 - (lvl * 250);
            nodes[id].x = (1000 / index) + ((1000 / index) * k) + (130 * (lvl % 2));

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

        printf("> %s : %d %d\n", node_ptr->name, x1, y1);

        int x2, y2;

        while (node_ptr->children != 0x0)
        {
            node_ptr = node_ptr->children;

            for (int j = 0; j < node_ptr; j++)
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

void draw_traverse(HEZDIMAGE hDib, HEZDFONT hFont, struct Node node, struct Node *nodes, int nodes_cnt, int x, int y)
{
    node.y = 950 - (node.level * 200);
    node.x = 200;

    struct Node *node_ptr = &node;

    // while(node_ptr->children != 0x0){
    //     node_ptr = node_ptr->children;
    //     for (int i = 0; i < nodes_cnt; i++)
    //     {
    //         if (strcmp(nodes[i].name, node_ptr->name) == 0)
    //         {
    //             if (nodes[i].level < lvl)
    //             {
    //                 nodes[i].level = lvl + 1;
    //                 node_ptr->level = lvl + 1;
    //                 traverse_nd(nodes, nodes[i], lvl + 1, nodes_cnt);
    //             }
    //             else
    //             {
    //                 node_ptr->level = nodes[i].level;
    //             }
    //             break;
    //         }
    //     }
    // }
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
                if (nodes[i].level < lvl)
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
    for (int i = 0; i < size; i++)
    {
        printf("%s : %s\n", datas[i][0], datas[i][1]);
    }
}

void print_nd(struct Node *nodes, int size)
{
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