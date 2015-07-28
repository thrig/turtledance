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

// PORTABILITY: Linux - install and include and compile with libbsd;
// otherwise, altagoobingleduckgo for file:zhelper.h to see a (bad) use
// of the (bad) rand() call, or so forth (or remove the rand code,
// there's not much, and it is optional).
#define randof(num)  (int) arc4random_uniform(num)

// How many repeats to perform; depends on the value of some nearby
// token, as modulated and with a floor as set by these values.
// (these will need to be decreased for some inputs, and raised for
// others, depending on the complexity of the input)
#define REPEAT_MOD 4
#define REPEAT_MIN 1

// Commands to emit for not-REPEAT bits of the tree; the Mods allow
// the various values to be reduced into a suitable set, e.g. there
// only being 16 colors, or what. FD/BK change the scale of the
// graph, as these indicate how far the turtle will move.
#define NUMCMDS 5
const int Mods[NUMCMDS] = { 3, 7, 59, 16, 3 };
const int Mult[NUMCMDS] = { 2, 0, 0, 0, 0 };
const bool Rand[NUMCMDS] = { 1, 0, 0, 0, 1 };

// another command to try is LABEL though that might require special
// handling to get the desired characters printed
const char *Commands[NUMCMDS] = { "FD", "RT", "LT", "SETPC", "BK" };

unsigned long Node_Count;
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
void treewalk(struct node *n, int depth);

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
            //n->value = EOF;
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
            if (c == prev_c)
                break;
            n = mknode(c);
            Cur_Node->next = n;
            n->prev = Cur_Node;
            Cur_Node = n;
            prev_c = c;
        }
    }

    printf("#!/usr/bin/env logo\nCS\nHT\n");
    printf("MAKE \"MY.POS POS\n");
    treewalk(Tree, 0);

    fprintf(stderr, "info: cmd stats nodes=%lu ", Node_Count);
    for (unsigned long i = 0; i < NUMCMDS; i++) {
        fprintf(stderr, "%lu ", Stats[i]);
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

    Node_Count++;

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

void treewalk(struct node *n, int depth)
{
    bool pen;
    int cmdidx, v, rep;

    if (n->child != NULL) {
        if (n->value != EOF) {
            printf("MAKE \"MY.POS POS\n");
            //printf("PENDOWN\n");
            Pen_Up = false;
            rep = abs((n->value + depth) % REPEAT_MOD) + REPEAT_MIN;
            if (rep < 1) rep = 1;
            printf("REPEAT %d [", rep);
        }

        treewalk(n->child, depth + 1);

        if (n->value != EOF) {
            printf("]\n");
            // need pen up here unless you want long lines as the turtle
            // resets back to where this repeat started at...
            printf("PENUP\n");
            Pen_Up = true;

            // restore whence started this repeat
            //printf("SETPOS MY.POS\n");
            // another option is to randomize or otherwise determine a
            // new location to jump to (produces circles in-place)
            printf("SETPOS (LIST %d %d)\n", (n->value & 15) * 30 - 225,
                   ((n->value >> 3) & 15) * 30 - 225);
            // or disable both, turtle will thus wander...

            // unhappy results if using XQuartz.app and remote-X11 to
            // OpenBSD box (as MacPorts ucblogo segfaults)
            //printf("FILL\n");
        }
    } else {
        if (n->value != EOF) {
            pen = n->value & 1;
            if (pen != Pen_Up) {
                printf("%s\n", pen ? "PENUP" : "PENDOWN");
                Pen_Up = pen;
            }
            cmdidx = abs((n->value + depth) % NUMCMDS);
            v = abs(n->value);
            if (Mods[cmdidx] > 0) v %= Mods[cmdidx];
            if (Mult[cmdidx] != 0) v *= Mult[cmdidx];
            if (Rand[cmdidx]) v = 1 + randof(v);
            if (cmdidx !=3) v += depth;
            printf("%s %d\n", Commands[cmdidx], v);

            // this seems beneficial
            //if (cmdidx == 0) printf("LEFT 1\n");

            Stats[cmdidx]++;
        }
    }

    if (n->next != NULL)
        treewalk(n->next, depth);
}
