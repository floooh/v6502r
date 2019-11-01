#pragma once
// machine generated, don't edit!
#include <stdint.h>
static const uint16_t seg_max_x = 8983; // max x coordinate (min is 0)
static const uint16_t seg_max_y = 9807; // max y coordinate (min is 0)
static const uint16_t grid_cells = 128; // length of picking grid in one dimension
extern uint16_t seg_vertices_0[76512]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_1[302628]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_2[1260]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_3[142788]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_4[72780]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_5[404376]; // (x,y,u=node_index,v=0) as triangle list
extern uint32_t pick_tris[304553][2]; // (layer,tri_index) pairs for picking check
extern uint32_t pick_grid[16384][2]; // [y*grid_cells+x](start,num) pairs into pick_tris
