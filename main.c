// How to use (requires linux with imagemagick and FFmpeg):

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef int bool;
int quad_capacity = 5; // TWEAKABLE (preformance)
int point_count = 10000; // TWEAKABLE
float G = 0.00001; // TWEAKABLE
float point_radius = 0.0001;
float collision_strength = 1.0;

int screen_width = 1000;
int screen_hight = 1000;

bool width_is_max;

void expect(void *p) {
    if (p == NULL) {
        printf("Bad alloc\n");
        exit(1);
    }
}

float rand_float() {
    return (float)rand()/(float)(RAND_MAX);
}

typedef struct {
    float x, y, vx, vy;
} Point;

void new_point(Point *p, float x, float y) {
    p->x = x;
    p->y = y;
    p->vx = p->vy = 0;
}

typedef struct {
    float x, y, w, h;
} Rectangle;

bool rect_contains_point(Rectangle *r, Point *p) {
    return (r->x <= p->x) & (p->x < (r->x+r->w)) &
           (r->y <= p->y) & (p->y < (r->y+r->h));
}

typedef struct {
    Rectangle bounds;
    int mass; // number of points in all child trees
    
    bool is_branch; // whether this is split or not
    void *ptr; // ethier a list of points or four quad tree branches
    int points; // stores the number of points if leave node
} QuadTree;

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
        // add to all child nodes
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

int min_int(int x, int y) {
    if (x < y) {return x;} else {return y;}
}

int range(int x, int value, int y) {
    return min_int(max_int(x, value), y);
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

void get_points_from_tree(QuadTree *qt, Point *array, int *index) {
    if (qt->is_branch) {
        QuadTree *children = qt->ptr;
        get_points_from_tree(&children[0], array, index);
        get_points_from_tree(&children[1], array, index);
        get_points_from_tree(&children[2], array, index);
        get_points_from_tree(&children[3], array, index);
    } else {
        Point *points = qt->ptr;
        for (int i=0; i<qt->points; i++) {
            array[*index] = points[i];
            *index += 1;
        }
    }
}

typedef struct {
    QuadTree **ptr;
    int size;
    int capacity;
} VecPointers;

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
    QuadTree **ptr = &(vec->ptr[vec->size]);
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

float distance_sqrd(float x, float y, float a, float b) {
    return ((x+a)*(x+a))+((y+b)*(y+b));
}

void update_point(Point *p, QuadTree *root) {
    VecPointers stack;
    vec_with_capacity(&stack, 100);
    QuadTree **pointer = vec_push(&stack);
    *pointer = root;
    VecPointers next_stack;
    vec_with_capacity(&next_stack, stack.size);
    while (1) {
        for (int i=0; i<stack.size; i++) {
            QuadTree *current_qt = get_vec(&stack, i);
            if (current_qt->is_branch) {
                // check if it's acceptable
                float center_x = current_qt->bounds.x+(current_qt->bounds.w/2.0);
                float center_y = current_qt->bounds.y+(current_qt->bounds.h/2.0);
                float size;
                if (width_is_max) {
                    size = current_qt->bounds.w;
                } else {
                    size = current_qt->bounds.h;
                }
                float acceptability = fabsf(distance(p->x, p->y, center_x, center_y) / size);
                if (acceptability > 2.0) { // TWEAKABLE (preformace, but a higher number may be less accurate)
                    // do calculations for the entire mass
                    float r = 1.0/distance_sqrd(p->x, p->y, center_x, center_y);
                    float x_part = center_x - p->x;
                    float y_part = center_y - p->y;
                    p->vx += r*x_part * G * current_qt->mass;
                    p->vy += r*y_part * G * current_qt->mass;
                } else {
                    // otherwise just add it's children to the stack
                    QuadTree *children = current_qt->ptr;
                    for (int x=0; x<4; x++) {
                        QuadTree **pointer = vec_push(&next_stack);
                        *pointer = &children[x];
                    }
                }
            } else {
                // do all the calculations with the points
                Point *points = current_qt->ptr;
                for (int j=0; j<current_qt->points; j++) {
                    float point_distance = distance_sqrd(p->x, p->y, points[j].x, points[j].y);
                    if (point_distance < point_radius*point_radius) { // point collision
                        float midpoint_x = (p->x + points[j].x) / 2.0;
                        float midpoint_y = (p->y + points[j].y) / 2.0;
                        p->vx += (midpoint_x + point_radius * (p->x - points[j].x) / point_distance) * collision_strength;
                        p->vy += (midpoint_y + point_radius * (p->y - points[j].y) / point_distance) * collision_strength;
                    }
                    float r = 1.0/point_distance;
                    float x_part = points[j].x - p->x;
                    float y_part = points[j].y - p->y;
                    p->vx += r*x_part * G;
                    p->vy += r*y_part * G;
                }
            }
        }
        // if there are no new jobs, break
        if (stack.size == 0) {
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

void apply_velocities(Point *points) {
    for (int i=0; i<point_count; i++) {
        points[i].x += points[i].vx * G;
        points[i].y += points[i].vy * G;
    }
}

void points_to_image(Point *points, unsigned char *image) {
    for (int i=0; i<point_count; i++) {
        int pixel_x = (points[i].x+1.0)*(float)(screen_width)*0.5;
        int pixel_y = (points[i].y+1.0)*(float)(screen_hight)*0.5;
        pixel_x = range(0, pixel_x, screen_width-1);
        pixel_y = range(0, pixel_y, screen_hight-1);
        image[(pixel_x+pixel_y*screen_width)*3] = 255;
        image[(pixel_x+pixel_y*screen_width)*3+1] = 255; // fabsf(points[i].vx)*50
        image[(pixel_x+pixel_y*screen_width)*3+2] = 255; // fabsf(points[i].vy)*50;
    }
}

void write_to_ppm(unsigned char *image, char *filename) {
    FILE *fptr;
    fptr = fopen(filename, "w");
    
    if (fptr == NULL) {
        printf("Could not make file: %s", filename);
        exit(1);
    }

    fprintf(fptr, "P3\n%d %d\n255\n", screen_width, screen_hight);
    for (int i=0; i<screen_hight*screen_width*3; i+=3) {
        fprintf(fptr, "%d %d %d\n", image[i], image[i+1], image[i+2]);
    }

    fclose(fptr); 
}

int main() {

    Point *points = calloc(point_count, sizeof(Point));
    expect(points);

    unsigned char *image = calloc(screen_hight*screen_width*3, sizeof(char));
    expect(image);

    char *filename = calloc(255, sizeof(char));
    expect(filename);

    int i=0;
    while (i<point_count) {
        float x = rand_float()*2.0-1.0;
        float y = rand_float()*2.0-1.0;
        float distance = x*x+y*y;
        if (distance < 1.0 & distance > 0.1) {
            Point p;
            p.x = x;
            p.y = y;
            // TODO: use actuall trig or something
            float velocity = G*point_count/distance * 1000; // TWEAKABLE (just the constant)
            velocity = 0.0;
            p.vx = y*velocity;
            p.vy = -x*velocity;
            points[i] = p;
            i++;
        }
    }

    int iteration = 0;

    while (1) {

        sprintf(filename, "%d.ppm", iteration);
        
        QuadTree root;
        new_quad_tree(&root);
        root.bounds.x = root.bounds.y = -1.0;
        root.bounds.w = root.bounds.h = 2.0;
        width_is_max = 1;

        for (int i=0; i<point_count; i++) {
            add_quad_tree_point(&root, &points[i]);
        }
    
        update_velocities(&root, &root);
        
        int index = 0;
        get_points_from_tree(&root, points, &index);
        point_count = index; // some might fly out and no longer be valid
        if (point_count == 0 ) {
            exit(0);
        }
        
        apply_velocities(points);
        
        memset(image, 0, screen_hight*screen_width*3*sizeof(char));
        points_to_image(points, image);
        write_to_ppm(image, filename);

        free_quad_tree(&root);

        printf("iteration %d : %d points\n", iteration, point_count);
        iteration++;
    }

    free(points);
    free(image);

    return 0;
}