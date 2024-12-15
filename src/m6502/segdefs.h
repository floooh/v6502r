#pragma once
// machine generated, don't edit!
#include <stdint.h>
static const uint16_t seg_max_x = 8984; // max x coordinate
static const uint16_t seg_max_y = 9808; // max y coordinate
static const uint16_t seg_min_x = 215; // min x coordinate
static const uint16_t seg_min_y = 180; // min y coordinate
static const uint16_t grid_cells = 128; // length of picking grid in one dimension
extern uint16_t seg_vertices_0[76536]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_1[301488]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_2[1260]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_3[142572]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_4[72516]; // (x,y,u=node_index,v=0) as triangle list
extern uint16_t seg_vertices_5[404268]; // (x,y,u=node_index,v=0) as triangle list
extern uint32_t pick_tris[316745][2]; // (layer,tri_index) pairs for picking check
extern uint32_t pick_grid[16384][2]; // [y*grid_cells+x](start,num) pairs into pick_tris
