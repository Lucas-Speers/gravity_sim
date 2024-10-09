#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

typedef int bool;
int quad_capacity = 1000;

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
    return (r->x <= p->x) & (p->x < (r->x+r->w)) &
           (r->y <= p->y) & (p->y < (r->y+r->h));
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
        // add to all child nodes TODO
        QuadTree *children = qt->ptr;
        add_quad_tree_point(&children[0], p);
        add_quad_tree_point(&children[1], p);
        add_quad_tree_point(&children[2], p);
        add_quad_tree_point(&children[3], p);
        qt->mass++;
    } else {
        if (qt->points == quad_capacity) {
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
            children[0].bounds.w = qt->bounds.w/2.0;
            children[0].bounds.h = qt->bounds.h/2.0;
            new_quad_tree(&children[1]);
            children[1].bounds.x = qt->bounds.x + qt->bounds.w/2.0;
            children[1].bounds.y = qt->bounds.y;
            children[1].bounds.w = qt->bounds.w/2.0;
            children[1].bounds.h = qt->bounds.h/2.0;
            new_quad_tree(&children[2]);
            children[2].bounds.x = qt->bounds.x;
            children[2].bounds.y = qt->bounds.y + qt->bounds.h/2.0;
            children[2].bounds.w = qt->bounds.w/2.0;
            children[2].bounds.h = qt->bounds.h/2.0;
            new_quad_tree(&children[3]);
            children[3].bounds.x = qt->bounds.x + qt->bounds.w/2.0;
            children[3].bounds.y = qt->bounds.y + qt->bounds.h/2.0;
            children[3].bounds.w = qt->bounds.w/2.0;
            children[3].bounds.h = qt->bounds.h/2.0;
            
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
            // simply add the point to the list
            Point *pointer = qt->ptr;
            pointer[qt->points] = *p;
            qt->points++;
            qt->mass++;
        }
    }
}

int max_int(int x, int y) {
    if (x > y) {return x;} else {return y;}
}

int max_depth(QuadTree *qt) {
    if (qt->is_branch) {
        int maximum_depth = 0;
        QuadTree *children = qt->ptr;
        maximum_depth = max_int(maximum_depth, max_depth(&children[0]));
        maximum_depth = max_int(maximum_depth, max_depth(&children[1]));
        maximum_depth = max_int(maximum_depth, max_depth(&children[2]));
        maximum_depth = max_int(maximum_depth, max_depth(&children[3]));
        return maximum_depth + 1;
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

struct VecPointers {
    QuadTree **ptr;
    int size;
    int capacity;
};
typedef struct VecPointers VecPointers;

void vec_with_capacity(VecPointers *vec, int capacity) {
    vec->ptr = calloc(capacity, sizeof(QuadTree*));
    expect(vec->ptr);
    vec->size = 0;
    vec->capacity = capacity;
}

QuadTree** vec_push(VecPointers *vec) {
    if (vec->size+1 == vec->capacity) {
        vec->capacity += 50;
        vec->ptr = realloc(vec->ptr, vec->capacity * sizeof(QuadTree*));
        expect(vec->ptr);
    }
    // printf("pointer1: %p\n", vec->ptr);
    QuadTree **ptr = &(vec->ptr[vec->size]);
    // printf("pointer2: %p\n", ptr);
    vec->size++;
    return ptr;
}

QuadTree* get_vec(VecPointers *vec, int index) {
    if (index >= vec->size) {
        printf("bruh\n");
        exit(1);
    }
    return vec->ptr[index];
}

void clear_vec(VecPointers *vec) {
    free(vec->ptr);
    vec->ptr = NULL;
    vec->size = 0;
}

float distance(float x, float y, float a, float b) {
    return sqrtf(((x+a)*(x+a))+((y+b)*(y+b)));
}

void update_point(Point *p, QuadTree *root) {
    VecPointers stack;
    vec_with_capacity(&stack, 50);
    QuadTree **pointer = vec_push(&stack);
    *pointer = root;
    VecPointers next_stack;
    vec_with_capacity(&next_stack, stack.size);
    while (1) {
        for (int i=0; i<stack.size; i++) {
            if (get_vec(&stack, i)->is_branch) {
                // check if it's acceptable TODO
                float acceptability = distance(p->x, p->y, get_vec(&stack, i)->bounds.x+(get_vec(&stack, i)->bounds.w/2.0), get_vec(&stack, i)->bounds.y+(get_vec(&stack, i)->bounds.h/2.0)) / max_int(get_vec(&stack, i)->bounds.w, get_vec(&stack, i)->bounds.h);
                if (acceptability < 0.5) {
                    // do calculations for the entire mass
                } else {
                    // otherwise just add it's children to the stack TODO
                    QuadTree *children = get_vec(&stack, i)->ptr;
                    for (int x=0; x<4; x++) {
                        QuadTree **pointer = vec_push(&next_stack);
                        *pointer = &children[x];
                    }
                }
            } else {
                // do all the calculations with the points TODO
            }
        }
        // if there are no new jobs, break
        if (stack.size == 0) {
            // printf("z\n");
            clear_vec(&stack);
            clear_vec(&next_stack);
            break;
        }

        // otherwise, rotate the stacks
        clear_vec(&stack);
        stack = next_stack;
        vec_with_capacity(&next_stack, stack.size);
    }
}

void update_velocities(QuadTree *root, QuadTree *current) {
    if (current->is_branch) {
        QuadTree *children = current->ptr;
        update_velocities(root, &children[0]);
        update_velocities(root, &children[1]);
        update_velocities(root, &children[2]);
        update_velocities(root, &children[3]);
    } else {
        Point *points = current->ptr;
        for (int i=0; i<current->points; i++) {
            update_point(&points[i], root);
        }
    }
}

int main() {
    printf("Starting...\n");
    
    QuadTree root;
    new_quad_tree(&root);
    
    root.bounds.x = root.bounds.y = -1.0;
    root.bounds.w = root.bounds.h = 2.0;
    
    for (int i=0; i<1000000; i++) {
        float x = rand_float()*2.0-1.0;
        float y = rand_float()*2.0-1.0;
        Point p;
        p.x = x;
        p.y = y;
        p.vx = p.vy = 0;
        add_quad_tree_point(&root, &p);
    }
    
    printf("Depth: %d\n", max_depth(&root));

    update_velocities(&root, &root);
    
    free_quad_tree(&root);
    
    return 0;
}