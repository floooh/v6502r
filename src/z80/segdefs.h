#pragma once
// machine generated, don't edit!
#include <stdint.h>
static const uint16_t seg_max_x = 9399; // max x coordinate (min is 0)
static const uint16_t seg_max_y = 9998; // max y coordinate (min is 0)
static const uint16_t grid_cells = 128; // length of picking grid in one dimension
extern uint16_t seg_vertices_0[248520]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_1[592548]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_2[0]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_3[300696]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_4[167796]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_5[865560]; // (x,y,u=node_index,v=0) as triangle list
extern uint32_t pick_tris[517756][2]; // (layer,tri_index) pairs for picking check
extern uint32_t pick_grid[16384][2]; // [y*grid_cells+x](start,num) pairs into pick_tris
