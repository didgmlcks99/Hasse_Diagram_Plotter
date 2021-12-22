# Hasse_Diagram_Plotter - buildscript

## Program Description

### This program reads and analyzes a '.dat' file that contains information data of edges and its vertices pair in order to plot a Hasse Diagram of this edges and vertices and give an output as a '(input file name).bmp' file visually.

<br>

## Inputs

### The input for this program is a .dat file containing a list of edges with its vertice pair in the format of "vertex1_vertex2" (former precedes latter vertex). A vertex is limited to be up to 16 alphanumeric characters at max. The program also limits to have only up to 32 vertices maximum.

<br>

## Compiling and Run
* 1. 'make' : run the command 'make' on the terminal to compile and make a executable file of 'hasse'.
* 2. './hasse <file>' : run the executable file created from step 1 with the command './hasse' with inclusion of the correct argument of the name of the '.dat' file.

<br>

## Makefile
* This file automatically runs the command for compiling the 'hasse.c' file with the right supporting '.c' files needed.