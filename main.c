#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

typedef int bool;
int quad_capacity = 50;

void expect(void *p) {
    if (p == NULL) {
        fprintf(stderr, "Bad alloc");
        exit(1);
    }
}

float rand_float() {
    return (float)rand()/(float)(RAND_MAX);
}

struct Point {
    float x, y, vx, vy;
};
typedef struct Point Point;

void new_point(Point *p, float x, float y) {
    p->x = x;
    p->y = y;
    p->vx = p->vy = 0;
}

struct Rectangle {
    float x, y, w, h;
};
typedef struct Rectangle Rectangle;

bool rect_contains_point(Rectangle *r, Point *p) {
    return
    (r->x <= p->x) & (p->x < r->x+r->w) &
    (r->y <= p->y) & (p->y < r->y+r->h);
}

struct QuadTree {
    Rectangle bounds;
    int mass; // number of points in all child trees
    
    bool is_branch; // whether this is split or not
    void *ptr; // ethier a list of points or four quad tree branches
    int points; // stores the number of points if leave node
};
typedef struct QuadTree QuadTree;

void new_quad_tree(QuadTree *qt) {
    qt->mass = 0;
    
    qt-> is_branch = 0;
    qt->points = 0;
    qt->ptr = calloc(quad_capacity, sizeof(Point));
    expect(qt->ptr);
}

void add_quad_tree_point(QuadTree *qt, Point *p) {
     // reject point if not in bounds
    if (!rect_contains_point(&qt->bounds, p)) {return;}
    
    if (qt->is_branch) {
        printf("a");
        // add to all child nodes TODO
        QuadTree *children = qt->ptr;
        add_quad_tree_point(&children[0], p);
        add_quad_tree_point(&children[1], p);
        add_quad_tree_point(&children[2], p);
        add_quad_tree_point(&children[3], p);
        qt->mass++;
    } else {
        if (qt->points == quad_capacity) {
            printf("b");
            // save the points to add them to child nodes
            Point *current_points = qt->ptr;
            
            // turn the node into a quad
            qt->is_branch = 1;
            qt->ptr = calloc(4, sizeof(QuadTree));
            expect(qt->ptr);
            
            // generating the new quads
            QuadTree *children = qt->ptr;
            new_quad_tree(&children[0]);
            children[0].bounds.x = qt->bounds.x;
            children[0].bounds.y = qt->bounds.y;
            children[0].bounds.w = qt->bounds.w/2;
            children[0].bounds.h = qt->bounds.h/2;
            new_quad_tree(&children[1]);
            children[0].bounds.x = qt->bounds.x + qt->bounds.w/2;
            children[0].bounds.y = qt->bounds.y;
            children[0].bounds.w = qt->bounds.w/2;
            children[0].bounds.h = qt->bounds.h/2;
            new_quad_tree(&children[2]);
            children[0].bounds.x = qt->bounds.x;
            children[0].bounds.y = qt->bounds.y + qt->bounds.h/2;
            children[0].bounds.w = qt->bounds.w/2;
            children[0].bounds.h = qt->bounds.h/2;
            new_quad_tree(&children[3]);
            children[0].bounds.x = qt->bounds.x + qt->bounds.w/2;
            children[0].bounds.y = qt->bounds.y + qt->bounds.h/2;
            children[0].bounds.w = qt->bounds.w/2;
            children[0].bounds.h = qt->bounds.h/2;
            
            // adding all the previous points
            for (int i=0; i<quad_capacity; i++) {
                add_quad_tree_point(&children[0], &current_points[i]);
                add_quad_tree_point(&children[1], &current_points[i]);
                add_quad_tree_point(&children[2], &current_points[i]);
                add_quad_tree_point(&children[3], &current_points[i]);
            }
            free(current_points);
            
            // now that we split the tree, just add the point and it will propagate
            add_quad_tree_point(qt, p);
        }
        else {
            printf("c");
            // simply add the point to the list
            Point *pointer = qt->ptr;
            pointer[qt->points] = *p;
            qt->points++;
            qt->mass++;
        }
    }
}

int max(int x, int y) {
    if (x > y) {return x;} else {return y;}
}

int max_depth(QuadTree *qt) {
    if (qt->is_branch) {
        int maximum_depth = 0;
        QuadTree *children = qt->ptr;
        maximum_depth = max(maximum_depth, max_depth(&children[0]));
        maximum_depth = max(maximum_depth, max_depth(&children[1]));
        maximum_depth = max(maximum_depth, max_depth(&children[2]));
        maximum_depth = max(maximum_depth, max_depth(&children[3]));
        return maximum_depth;
    }
    return 1;
}

void free_quad_tree(QuadTree *qt) {
    if (qt->is_branch) {
        QuadTree *children = qt->ptr;
        free_quad_tree(&children[0]);
        free_quad_tree(&children[1]);
        free_quad_tree(&children[2]);
        free_quad_tree(&children[3]);
    }
    free(qt->ptr);
}

int main() {
    printf("Hello World!\n");
    
    QuadTree root;
    new_quad_tree(&root);
    
    root.bounds.x = root.bounds.y = -1.0;
    root.bounds.w = root.bounds.h = 2.0;
    
    for (int i=0; i<10000; i++) {
        printf(".");
        float x = rand_float()*2.0-1.0;
        float y = rand_float()*2.0-1.0;
        Point p;
        p.x = x;
        p.y = y;
        p.vx = p.vy = 0;
        add_quad_tree_point(&root, &p);
    }
    
    printf("Depth: %d\n", max_depth(&root));
    
    free_quad_tree(&root);
    
    return 0;
}