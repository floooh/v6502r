#pragma once
// machine generated, don't edit!
#include <stdint.h>
static const uint16_t seg_max_x = 12633; // max x coordinate
static const uint16_t seg_max_y = 12423; // max y coordinate
static const uint16_t seg_min_x = 2; // min x coordinate
static const uint16_t seg_min_y = 2; // min y coordinate
static const uint16_t grid_cells = 128; // length of picking grid in one dimension
extern uint16_t seg_vertices_0[185136]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_1[786120]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_2[6972]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_3[374628]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_4[197208]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_5[1115688]; // (x,y,u=node_index,v=0) as triangle list
extern uint32_t pick_tris[429373][2]; // (layer,tri_index) pairs for picking check
extern uint32_t pick_grid[16384][2]; // [y*grid_cells+x](start,num) pairs into pick_tris
