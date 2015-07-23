/*
 * turtledance. Emits, uh, dance steps.
 *
 * Too much input will cause malloc() to fail. Do not do that, and feed
 * in less input, or maintain a count of nodes and set some upper limit
 * on how many to create.
 */

#include <err.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

// How many repeats to perform; depends on the value of some nearby
// token, as modulated and with a floor as set by these values.
#define REPEAT_MOD 16
#define REPEAT_MIN 4

// Commands to emit for not-REPEAT bits of the tree; the Mods allow
// the various values to be reduced into a suitable set, e.g. there
// only being 16 colors, or what. FD/BK change the scale of the
// graph, as these indicate how far the turtle will move.
#define NUMCMDS 5
const int Mods[NUMCMDS] = { 7, 60, 60, 16, 3 };
const int Mult[NUMCMDS] = { 0, 0, 0, 0, 0 };
const bool Rand[NUMCMDS] = { 0, 0, 0, 0, 0 };
const char *Commands[NUMCMDS] = { "FD", "RT", "LT", "SETPC", "BK" };

unsigned long Stats[NUMCMDS];

bool Pen_Up;
int Cur_Color;

struct node {
    int value;
    struct node *prev;
    struct node *next;
    struct node *parent;
    struct node *child;
};

struct node *Tree, *Cur_Node;

struct node *mknode(int value);
struct node *parent(struct node *n);
void treewalk(struct node *n);

int main(void)
{
    int c, prev_c = EOF;
    struct node *n;

    Tree = mknode(EOF);
    Cur_Node = Tree;

    while ((c = getchar()) != EOF) {
        switch (c) {
        case '(':
        case '{':
        case '<':
        case '[':
            // braces cause repeat statements around whatever is contained
            // by them
            n = mknode(c);
            n->value = EOF;
            Cur_Node->child = n;
            n->parent = Cur_Node;
            Cur_Node = n;
            break;

        case ')':
        case '}':
        case '>':
        case ']':
            // back out of the repeat...
            Cur_Node = parent(Cur_Node);
            break;

        default:
            if (c == prev_c) break;
            n = mknode(c);
            Cur_Node->next = n;
            n->prev = Cur_Node;
            Cur_Node = n;
            prev_c = c;
        }
    }

    printf("#!/usr/bin/env logo\nCS\nHT\n");
    treewalk(Tree);

    fprintf(stderr, "info: cmd stats ");
    for (unsigned long i = 0; i < NUMCMDS; i++) {
        fprintf(stderr, "%d ", Stats[i]);
    }
    fprintf(stderr, "\ninfo: C-c to stop ucblogo, \"bye\" to exit it.\n");

    exit(EXIT_SUCCESS);
}

struct node *mknode(int value)
{
    struct node *n;

    if ((n = malloc(sizeof(struct node))) == NULL)
        err(EX_OSERR, "could not malloc() node");

    n->prev = NULL;
    n->next = NULL;
    n->parent = NULL;
    n->child = NULL;
    n->value = value;

    return n;
}

struct node *parent(struct node *n)
{
    struct node *cur;

    // has a direct parent
    if (n->parent != NULL)
        return n->parent;

    // or instead parent is someways back along a number of siblings
    cur = n;
    while (cur->prev != NULL) {
        if (cur->prev->parent != NULL)
            return cur->prev->parent;
        cur = cur->prev;
    }

    // no parent? back up to eldest sibling and pretend all is well
    return cur;
}

void treewalk(struct node *n)
{
    bool pen;
    int cmdidx, v, rep;

    if (n->child != NULL) {
        if (n->value != EOF) {
            printf("MAKE \"MY.POS POS\n");
            //printf("PENDOWN\n");
            Pen_Up = false;
            rep = abs(n->value % REPEAT_MOD) + REPEAT_MIN;
            if (rep < 1)
                rep = 1;
            printf("REPEAT %d [", rep);
        }

        treewalk(n->child);

        if (n->value != EOF) {
            printf("]\n");
            // need pen up here unless you want long lines as the turtle
            // resets back to where this repeat started at...
            printf("PENUP\n");
            printf("SETPOS MY.POS\n");
            //printf("FILL\n");
            Pen_Up = true;
        }
    } else {
        if (n->value != EOF) {
            pen = n->value & 1;
            if (pen != Pen_Up) {
                printf("%s\n", pen ? "PENUP" : "PENDOWN");
                Pen_Up = pen;
            }
            cmdidx = abs(n->value % NUMCMDS);
            v = abs(n->value);
            if (Mods[cmdidx] > 0)
                v %= Mods[cmdidx];
            if (Mult[cmdidx] != 0)
                v *= Mult[cmdidx];
            if (Rand[cmdidx])
                v = 1 + arc4random_uniform(v);
            printf("%s %d\n", Commands[cmdidx], v);

            // this seems beneficial
            if (cmdidx == 0) printf("LEFT 1\n");

            Stats[cmdidx]++;
        }
    }

    if (n->next != NULL)
        treewalk(n->next);
}
