/*
change file name
3 Camera

2. Spawn point and level camera.

Read THINGS type 1 for position and angle, and put a free-fly camera there so you can inspect the level.
The eye height needs the floor under the spawn. A simple point-in-sector test is enough for now.
The BSP lookup can wait for gameplay.

3. Lighting. This is the biggest design decision, and it should come before animation.

Today the light level is baked into each mesh vertex. Sector specials such as flicker, strobe and glow change
a sector’s light at runtime, so baking blocks them.
I’d propose a per-sector light table, uploaded as a small texture or uniform buffer, with each vertex
carrying a sector index. Changing a light then costs one table write and no mesh rebuild.
Then comes the fake contrast, where walls aligned to the x or y axis get a ±1 light level,
which is cheap once the table exists.
Then distance dimming. It needs the palette shader (Option B) with COLORMAP,
so we’d decide whether to switch to it here.

4. Sky. Draw SKY1 mapped by view angle as a background, using the sky ceilings and the skipped upper walls as the mask.

5. Visual animation.

Animated flats and wall textures, using the number remap we designed.
Scrolling walls (linedef special 48).
Sector light effects, which build on the light table from step 3.

6. Static things (optional here). Decorations such as lamps and barrels.
*/

struct Sector_Edge
{
	// indices to vertex values
    u32 v1;
    u32 v2;
};

struct Edge_Loop
{
    u32 *vertices;      // walk order, indices into the level's vertex array
    u32 vertex_count;

	// filled in by classify_loop
    f32 signed_area;
	u32 *hole_indices; // this data is only used by outer loops
	u32 hole_count;
};

struct Chained_Loops
{
    Edge_Loop *loops;
    u32 loop_count;
};

struct Classified_Sector_Loops
{
    Edge_Loop *outer;
    u32 outer_count;
    Edge_Loop *holes;
    u32 hole_count;
};

struct Bridge_Pair
{
    u32 outer_vertex_index;  // index into outer->vertices
    u32 hole_vertex_index;   // index into hole->vertices
    f32 dist_sq;
};

struct Triangle
{ 
	// vertex indices
	u32 a, b, c;
};

struct Triangulated_Loop
{
	Triangle *triangles;
	u32 triangle_count;
};

enum Wall_Part
{
	WallPart_Upper,
	WallPart_Lower,
	WallPart_Middle,
	WallPart_Masked_Middle
};

struct Wall_Segment
{
    Mesh mesh;
    Wall_Part part;
    u32 texture_number;
};

struct Wall_Segment_List
{
	Wall_Segment *segments;
	u32 segment_count;
};

// NOTE(Fermin): everything the wall builder needs to resolve a texture lazily.
struct Wall_Build_Context
{
	Texture_Cache *cache;
	Wad_File *wad;
	Platform_API *platform_API;
	Memory_Arena *scratch;   // compositing scratch space
};

enum Point_Location
{
	PointLocation_Outside,
	PointLocation_Inside,
	PointLocation_On_Boundary,
};

// NOTE(Fermin): hand-found gaps in the original DOOM.WAD data. A sector whose outline is
// missing a linedef gets a synthetic edge so it can be triangulated. Vanilla never notices
// because it renders through the BSP. Synthetic edges build no wall geometry.
struct Edge_Patch
{
	char *map_name;
	u32 sector_index;
	u32 from_vertex;
	u32 to_vertex;
};

static Edge_Patch edge_patches[] =
{
	{ "E1M3", 7, 771, 764 },
};

static b32
edge_patch_applies(Edge_Patch *patch, Wad_File *wad, i32 map_marker, u32 sector_index)
{
	// NOTE(Fermin): lump name is the unterminated 8 byte field, same argument order as find_lump
	b32 result;

	result =  (strings_are_equal_case_insensitive(wad->lumps[map_marker].name, patch->map_name, 8)) &&
			  (patch->sector_index == sector_index);

	return result;
}

static u32
count_edge_patches(Wad_File *wad, i32 map_marker, u32 sector_index)
{
	u32 result = 0;
	for(u32 i = 0; i < array_count(edge_patches); ++i)
	{
		if(edge_patch_applies(edge_patches + i, wad, map_marker, sector_index)) { ++result; }
	}
	return result;
}

// NOTE(Fermin): debug only. In a closed, consistent sector every vertex has
// in_degree == out_degree. Print the vertices where that fails.
static void
debug_check_edge_balance(Sector_Edge *edges, u32 edge_count, V2 *vp, u32 sector_index, Memory_Arena *arena)
{
	u32 max_v = 0;
	for(u32 i = 0; i < edge_count; ++i)
	{
		if(edges[i].v1 > max_v) { max_v = edges[i].v1; }
		if(edges[i].v2 > max_v) { max_v = edges[i].v2; }
	}

	i32 *balance = push_array(arena, max_v + 1, i32);
	for(u32 i = 0; i < edge_count; ++i)
	{
		balance[edges[i].v1] += 1; // outgoing
		balance[edges[i].v2] -= 1; // incoming
	}

	for(u32 v = 0; v <= max_v; ++v)
	{
		if(balance[v] != 0)
		{
			printf("sector %u: vertex %u (%d, %d) out-in = %d\n",
			       sector_index, v, (i32)vp[v].x, (i32)vp[v].y, balance[v]);
		}
	}
};

static u32
turn_class(V2 in, V2 out)
{
	/*
	 * NOTE(Fermin): turn class relative to the incoming direction.
	 * 0 = CW       -- cross <  0
	 * 1 = straight -- cross == 0 & inner > 0
	 * 2 = CCW		-- cross >  0
	 * 3 = U-turn	-- cross == 0 & inner < 0
	 * Lower is preferred. Origin doesn't matter, cross and inner only use
	 * a vector's direction and length, not its position. To visualize this
	 * imagin both vectors origin in the same point.
	 *
	 * cross(a, b) > 0 when b is CCW from a
	 * cross(a, b) < 0 when b is CW from a
	*/

	f64 cross_p = cross_f64(in, out);
	if(cross_p < 0.0) { return 0; }
	if(cross_p > 0.0) { return 2; }

	f64 inner_p = inner_f64(in, out);
	return (inner_p > 0.0) ? 1 : 3;
}

static b32
is_sharper_right(V2 in, V2 out, V2 best)
{
	/*
	* NOTE(Fermin): true if out vector a is a sharper right turn than best,
	* given both are relative to the same incoming direction.
	*/

	u32 class_out = turn_class(in, out);
	u32 class_best = turn_class(in, best);
	if(class_out != class_best) { return class_out < class_best; }

	// same class: out is the smaller angle if best is CCW of out
	return cross_f64(out, best) > 0.0;
}

#define INVALID_EDGE_INDEX 0xFFFFFFFF
static u32
pick_next_edge(Sector_Edge *edges, u32 current, u32 start, b32 *visited,
			   u32 *first_edge_at, u32 *next_at_start, V2 *vp)
{
	/* 
	* NOTE(Fermin): among the outgoing edges at edge->v2, pick the sharpest RIGHT(CW) turn
	* relative to the incoming direction (smallest signed angle; right = negative).
	* Only unvisited edges, or the loop's start edge, are candidates.
	*/

	Sector_Edge *cur = edges + current;
	V2 p0 = vp[cur->v1];
	V2 p1 = vp[cur->v2];
	V2 in = p1 - p0;

	u32 best = INVALID_EDGE_INDEX;
	V2 best_out = {};

	for(u32 e = first_edge_at[cur->v2]; e != INVALID_EDGE_INDEX; e = next_at_start[e])
	{
		if(visited[e] && e != start) { continue; }

		V2 q = vp[edges[e].v2];
		V2 out = q - p1;

		if(best == INVALID_EDGE_INDEX || is_sharper_right(in, out, best_out))
		{
			best = e;
			best_out = out;
		}
	}

	return best;
}

static Chained_Loops
chain_edges_to_loops(Sector_Edge *edges, u32 edge_count, V2 *vertex_positions, Memory_Arena *arena)
{
    Chained_Loops result = {};

    // NOTE(Fermin): find the vertex index range so first_edge_at can be a flat array
    u32 max_vertex_index = 0;
    for(u32 edge_index = 0; edge_index < edge_count; ++edge_index)
    {
        Sector_Edge *edge = edges + edge_index;
        if(edge->v1 > max_vertex_index) { max_vertex_index = edge->v1; }
        if(edge->v2 > max_vertex_index) { max_vertex_index = edge->v2; }
    }

    u32 *first_edge_at = push_array(arena, max_vertex_index + 1, u32);
    u32 *next_at_start = push_array(arena, edge_count, u32);
    for(u32 vertex_index = 0; vertex_index <= max_vertex_index; ++vertex_index)
    {
        first_edge_at[vertex_index] = INVALID_EDGE_INDEX;
    }
    for(u32 edge_index = 0; edge_index < edge_count; ++edge_index)
    {
		/*
		* first_edge_at: At starting vertex V we got this edge index.
		* Given the vertex you just arrived at, one array lookup tells you the next edge.
		* This works because edges[0].v2 = edges[1].v1, so you can use the edge v2 as
		* index to get the next edge.
		*
		* We need to handle multipe overlapping vertices starting on the same point.
		* first_edge_at stores only the last edge's index at that vertex.
		* next_at_start is a linked list that stores the next edge index also starting at V.
		* 
		* first_edge_at[v] is the head of vertex v's list of outgoing edges.
		* next_at_start[e] is the next edge after e that starts at the same vertex as e.
		* It's indexed by edge, not by vertex. Both start as X (INVALID_EDGE_INDEX)
		* or are overwritten before being read.
		*
		*/
		next_at_start[edge_index] = first_edge_at[edges[edge_index].v1]; // old head becomes my successor
        first_edge_at[edges[edge_index].v1] = edge_index; // I become the head
    }

    b32 *visited = push_array(arena, edge_count, b32);
	// @Cleanup: why edge_count edge loops? over-allocating. We cant know
	// the number of loops. Can we do better?
    Edge_Loop *loops = push_array(arena, edge_count, Edge_Loop); 

	// NOTE(Fermin): Walk each unvisited edge until it closes
    for(u32 start_edge_index = 0; start_edge_index < edge_count; ++start_edge_index)
    {
        if(visited[start_edge_index]) { continue; }

        Edge_Loop *loop = loops + result.loop_count++;
        *loop = {};
		// @Cleanup: upper bound, trim later if you care.
		// Over-allocating, edge_count is all loops' edges.
        loop->vertices = push_array(arena, edge_count, u32); 

        u32 current_edge_index = start_edge_index;
        for(;;)
        {
            assert(!visited[current_edge_index]);
            visited[current_edge_index] = true;

            Sector_Edge *current_edge = edges + current_edge_index;
            loop->vertices[loop->vertex_count++] = current_edge->v1;

			u32 next_edge_index = pick_next_edge(edges, current_edge_index, start_edge_index,
									  visited, first_edge_at, next_at_start, vertex_positions);

            assert(next_edge_index != INVALID_EDGE_INDEX); // open chain -> malformed sector data
            if(next_edge_index == start_edge_index) { break; }
            current_edge_index = next_edge_index;
        }
    }

    result.loops = loops;
    return result;
}

static f32
signed_area_for_loop(Edge_Loop *loop, V2 *vertex_positions)
{
	// NOTE(Fermin): Shoelace formula.
	// Counterclockwise is positive
    f32 area = 0.0f;
    for(u32 i = 0; i < loop->vertex_count; ++i)
    {
        V2 a = vertex_positions[loop->vertices[i]];
		// modulo because the last edge loops to the first vertex 
        V2 b = vertex_positions[loop->vertices[(i + 1) % loop->vertex_count]]; 
        //area += (a.x * b.y) - (b.x * a.y);
        area += cross(a, b);
    }

	// If we are only interested in the sign we could not do this division
    return 0.5f * area;
}

static b32
is_point_on_edge(V2 v0, V2 v1, V2 p)
{
	f64 c = ((f64)p.x - v0.x) * ((f64)v1.y - v0.y) -
	        ((f64)p.y - v0.y) * ((f64)v1.x - v0.x);

	if(c != 0.0) { return false; }

	return min(v0.x, v1.x) <= p.x && p.x <= max(v0.x, v1.x) &&
	       min(v0.y, v1.y) <= p.y && p.y <= max(v0.y, v1.y);
}

static Point_Location
locate_point_in_loop(Edge_Loop *loop, V2 p, V2 *vertex_positions)
{
	/*
	 * https://www.geeksforgeeks.org/dsa/how-to-check-if-a-given-point-lies-inside-a-polygon/
	*/

	b32 inside = false;

	for(u32 i = 0; i < loop->vertex_count; i++)
	{
		V2 v0 = vertex_positions[loop->vertices[(i + 0)]];
		V2 v1 = vertex_positions[loop->vertices[(i + 1) % loop->vertex_count]];

		if(is_point_on_edge(v0, v1, p)) { return PointLocation_On_Boundary; }

		f64 x_at_y = ((f64)v1.x - v0.x) * ((f64)p.y - v0.y) / ((f64)v1.y - v0.y) + v0.x;
		b32 intersect = ((v0.y > p.y) != (v1.y > p.y)) && ((f64)p.x < x_at_y);

		if(intersect) { inside = !inside; }
	}

	return inside ? PointLocation_Inside : PointLocation_Outside;
}

static b32
is_hole_inside_loop(Edge_Loop *hole, Edge_Loop *outer, V2 *vp)
{
	/*
	 * Boundary vertices (overlap of a hole's vertex and outer's edge) is possible,
	 * but inconclusive to determine whether the hole is inside or not.
	 * We search for the first non-boundary vertex and use that instead.
	*/

	for(u32 i = 0; i < hole->vertex_count; i++)
	{
		Point_Location loc = locate_point_in_loop(outer, vp[hole->vertices[i]], vp);
		if(loc != PointLocation_On_Boundary) { return loc == PointLocation_Inside; }
	}

	return false;
}

static Classified_Sector_Loops
classify_loops(Chained_Loops *chained, V2 *vertex_positions, Memory_Arena *arena)
{
    Classified_Sector_Loops result = {};
	assert(chained->loop_count > 0); // every sector has at least one loop (the outer boundary)
	
	// @Cleanup: Can we be more precise with these allocations?
    result.outer = push_array(arena, chained->loop_count, Edge_Loop); 
    result.holes = push_array(arena, chained->loop_count, Edge_Loop); 

    for(u32 loop_index = 0; loop_index < chained->loop_count; ++loop_index)
    {
        Edge_Loop *loop = chained->loops + loop_index;
        loop->signed_area = signed_area_for_loop(loop, vertex_positions);

        if(loop->signed_area < 0.0f)
        {
            // NOTE(Fermin): convention -> negative area is the outer boundary
            result.outer[result.outer_count++] = *loop;
        }
        else
        {
            result.holes[result.hole_count++] = *loop;
        }
    }

	/*
	 * Its possible to have nested regions; an island inside a hole inside a big outer,
	 * in that case the hole is inside both island's outer and the big outer so we need
	 * to make sure each hole has only one owner so it doesn't get triangulated twice.
	 * The owner with the smallest area is the right one.
	*/
	for(u32 hole_index = 0; hole_index < result.hole_count; ++hole_index)
	{
		i32 best_outer = -1;
		f32 best_area = 0.0f;
		for(u32 o = 0; o < result.outer_count; ++o)
		{
			if(!is_hole_inside_loop(&result.holes[hole_index], &result.outer[o], vertex_positions)) { continue; }
			f32 area = fabsf(result.outer[o].signed_area);
			if(best_outer < 0 || area < best_area) { best_outer = (i32)o; best_area = area; }
		}
		if(best_outer >= 0)
		{
			Edge_Loop *o = &result.outer[best_outer];

			if(!o->hole_indices)
			{
				// We don't know how many more holes belong to this outer,
				// so we allocate enough for all the remaining holes
				o->hole_indices = push_array(arena, result.hole_count - hole_index, u32);
			}
			o->hole_indices[o->hole_count++] = hole_index;
		}
	}

	assert(result.outer_count > 0);

    return result;
}

static Bridge_Pair
find_closest_bridge_pair(Edge_Loop *outer, Edge_Loop *hole, V2 *vertex_positions)
{
    Bridge_Pair result = {};
    result.dist_sq = F32_MAX;

    for(u32 outer_i = 0; outer_i < outer->vertex_count; ++outer_i)
    {
        V2 outer_p = vertex_positions[outer->vertices[outer_i]];
        for(u32 hole_i = 0; hole_i < hole->vertex_count; ++hole_i)
        {
            V2 hole_p = vertex_positions[hole->vertices[hole_i]];
			V2 d = outer_p - hole_p;
            f32 dist_sq = (d.x * d.x) + (d.y * d.y);

            if(dist_sq < result.dist_sq)
            {
                result.dist_sq = dist_sq;
                result.outer_vertex_index = outer_i;
                result.hole_vertex_index = hole_i;
            }
        }
    }
    return result;
}

static Edge_Loop
merge_hole_into_outer(Edge_Loop *outer, Edge_Loop *hole, V2 *vertex_positions, Memory_Arena *arena)
{
	/*
	 * Create an Edge_Loop by mergin the outer loop with a hole through their
	 * closest vertices
     */

    Bridge_Pair bridge = find_closest_bridge_pair(outer, hole, vertex_positions);

    u32 merged_capacity = outer->vertex_count + hole->vertex_count + 2; // +2 for the duplicated seam vertices
    u32 *merged = push_array(arena, merged_capacity, u32);
    u32 merged_count = 0;

	// From start to bridge's outer index
    for(u32 i = 0; i <= bridge.outer_vertex_index; ++i)
        merged[merged_count++] = outer->vertices[i];

	// From hole's bridge index until loop
    for(u32 step = 0; step < hole->vertex_count; ++step)
    {
        u32 hole_i = (bridge.hole_vertex_index + step) % hole->vertex_count;
        merged[merged_count++] = hole->vertices[hole_i];
    }

    merged[merged_count++] = hole->vertices[bridge.hole_vertex_index];   // close the hole loop, walk back to bridge vertex
    merged[merged_count++] = outer->vertices[bridge.outer_vertex_index]; // close the seam, back onto outer

    for(u32 i = bridge.outer_vertex_index + 1; i < outer->vertex_count; ++i)
        merged[merged_count++] = outer->vertices[i];

    Edge_Loop result = {};
    result.vertices = merged;
    result.vertex_count = merged_count;
	result.hole_indices = outer->hole_indices;
	result.hole_count = outer->hole_count;

    return result;
}

/*
 *         c
 *        /.\
 *       / . \
 *      /  p  \
 *     /  . .  \
 *    / .     . \
 *    a ________ b
 *
 * if all 3 sub-triangles have the same sign then p is inside
*/
static b32
point_in_triangle(V2 p, V2 a, V2 b, V2 c)
{
    f32 d1 = cross((b - a), (p - a)); // which side of edge a→b is p on?
    f32 d2 = cross((c - b), (p - b)); // which side of edge b→c is p on?
    f32 d3 = cross((a - c), (p - c)); // which side of edge c→a is p on?

	// NOTE(Fermin): We need to check both because we have triangles wound
	// clockwise and counter clockwise
    b32 has_neg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
    b32 has_pos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);

    return !(has_neg && has_pos); // all same sign -> p is inside or on the edge
}

/*
*
* an ear is a vertex where you could slice off the triangle formed by it
* and its two neighbors, and that triangle is entirely part of the polygon's interior
*     _______
*     |\    |
*     | \   |
*     |  \  |
*     |   \ |
*     |ear \|
*     |_____\
*
* ears require two conditions:
* - convex. The interior angle at that vertex must be less than 180.
* - empty. Even if convex, no other vertex of the polygon can be sitting
*   inside that triangle.
*
*/
static b32
is_ear(u32 *ring, u32 ring_count, u32 i, V2 *vertex_positions)
{
	/*
	*
	* i is the index of the current vertex we're testing
	*
	*/
    u32 prev_i = (i + ring_count - 1) % ring_count;
    u32 next_i = (i + 1) % ring_count;

    u32 va = ring[prev_i];
	u32 vb = ring[i];
	u32 vc = ring[next_i];
    V2 a = vertex_positions[va];
    V2 b = vertex_positions[vb];
    V2 c = vertex_positions[vc];

	/*
	* Is it convex?
	* Since our convention is the front is always to the right/clockwise and
	* we navigate the polygon in clockwise order, if the result is negative
	* the angle is convex, and if positive it is concave.
	*/
    if(cross((b - a), (c - b)) >= 0.0f) { return false; }

	// is it empty?
    for(u32 j = 0; j < ring_count; ++j)
    {
        u32 vj = ring[j];

		// skip by vertex index, not ring index -- the bridge duplicates a vertex at two ring positions
        if(vj == va || vj == vb || vj == vc) { continue; } 

        if(point_in_triangle(vertex_positions[vj], a, b, c)) { return false; }
    }

    return true;
}

static Triangulated_Loop
triangulate_ear_clip(u32 *loop_vertices, u32 loop_vertex_count, V2 *vertex_positions, Memory_Arena *arena)
{
	/*
	 * clips a merged edge loop into triangles
	 *
	*/

    Triangulated_Loop result = {};
    result.triangles = push_array(arena, loop_vertex_count - 2, Triangle); // a n-gon always yields n-2 triangles

	/*
	* ring is the current working polygon boundary as a list of vertex indices.
	* It starts as all the loop vertices of the merged bridge and it
	* shrinks by one every time an ear gets clipped until 3 vertices
	* remain.
	*/
    u32 *ring = push_array(arena, loop_vertex_count, u32);
    for(u32 i = 0; i < loop_vertex_count; ++i) { ring[i] = loop_vertices[i]; }
    u32 ring_count = loop_vertex_count;

    u32 guard = 0;
    u32 max_iterations = loop_vertex_count * loop_vertex_count; // safety valve, shouldn't ever trip on valid data

    while(ring_count > 3 && guard++ < max_iterations)
    {
        b32 clipped_this_pass = false;
        for(u32 i = 0; i < ring_count; ++i)
        {
            if(is_ear(ring, ring_count, i, vertex_positions))
            {
                u32 prev_i = (i + ring_count - 1) % ring_count;
                u32 next_i = (i + 1) % ring_count;

                Triangle *tri = result.triangles + result.triangle_count++;
                tri->a = ring[prev_i];
                tri->b = ring[i];
                tri->c = ring[next_i];

                for(u32 j = i; j < ring_count - 1; ++j) { ring[j] = ring[j + 1]; } // shift out the clipped vertex
                --ring_count;

                clipped_this_pass = true;
                break; // restart the scan since ring indices shifted
            }
        }
        assert(clipped_this_pass); // no ear found -> malformed/self-intersecting polygon
    }

    Triangle *tri = result.triangles + result.triangle_count++;
    tri->a = ring[0]; tri->b = ring[1]; tri->c = ring[2];

    return result;
}

static Mesh
build_flat_mesh(Triangulated_Loop *tri, V2 *vertex_positions, f32 z, f32 light,
				b32 flip_winding, Memory_Arena *arena)
{
    Mesh result = {};
    result.vertex_count = tri->triangle_count * 3;
    result.vertices = push_array(arena, result.vertex_count, Mesh_Vertex);
    result.index_count = result.vertex_count;
    result.indices = push_array(arena, result.index_count, u32);

    for(u32 t = 0; t < tri->triangle_count; ++t)
    {
        Triangle *triangle = tri->triangles + t;
        u32 ids[3] = { triangle->a, triangle->b, triangle->c };
		// ceiling faces down, needs opposite winding from floor
        if(flip_winding) { u32 tmp = ids[0]; ids[0] = ids[2]; ids[2] = tmp; } 

        for(u32 k = 0; k < 3; ++k)
        {
            V2 p = vertex_positions[ids[k]];
            u32 out_index = t * 3 + k;

            Mesh_Vertex *v = result.vertices + out_index;
            v->position = { p.x, p.y, z };
            v->uv = { (p.x / 64.0f), (p.y / 64.0f) }; // DOOM flats are 64x64, world units map 1:1 to UV at that scale
            v->light = light;

            result.indices[out_index] = out_index;
        }
    }

    return result;
}

static Mesh
build_wall_quad(V2 p1, V2 p2, f32 bottom_z, f32 top_z, f32 light,
                 f32 x_offset, f32 anchor_top, f32 tex_w, f32 tex_h, Memory_Arena *arena)
{
    f32 wall_length = length(p2 - p1);
    f32 wall_height = top_z - bottom_z;

    V3 positions[4] = {
        { p1.x, p1.y, bottom_z },
        { p2.x, p2.y, bottom_z },
        { p2.x, p2.y, top_z },
        { p1.x, p1.y, top_z },
    };

    // NOTE(Fermin): DOOM textures are top-pegged by default -- v=0 at the top edge, growing downward
	f32 u0 = x_offset / tex_w;
	f32 u1 = (x_offset + wall_length) / tex_w;
	f32 v_bottom = (anchor_top - bottom_z) / tex_h;
	f32 v_top = (anchor_top - top_z) / tex_h;

    V2 uvs[4] = {
        { u0, v_bottom },
        { u1, v_bottom },
        { u1, v_top },
        { u0, v_top }
    };

    Mesh result = {};
    result.vertex_count = 4;
    result.vertices = push_array(arena, 4, Mesh_Vertex);
    for(u32 i = 0; i < 4; ++i)
    {
        result.vertices[i].position = positions[i];
        result.vertices[i].uv = uvs[i];
        result.vertices[i].light = light;
    }

	u32 idx[6] = { 0, 1, 2, 0, 2, 3 }; 
    result.index_count = 6;
    result.indices = push_array(arena, 6, u32);
    for(u32 i = 0; i < 6; ++i) { result.indices[i] = idx[i]; }

    return result;
}

static f32
wall_anchor_top(Wall_Part part, u16 flags, Sector *near_s, Sector *far_s, f32 tex_h)
{
	/*
	 * The 'TOP' and 'BOTTOM' on the flag names mean upper and lower wall respectively.
	 * Not the edge to which the texture is not pegged to.
	 *
	 * You can read these flags like so:
	 * ML_DONTPEGTOP == Dont use default pegging on top wall
	 * ML_DONTPEGBOTTOM == Dont use default pegging on bottom wall
	 *
	 * ML_DONTPEGBOTTOM is reused for middle walls with same meaning; dont do default pegging.
	 * It's confusing.
	 *
	*/

	switch(part)
	{
		case WallPart_Middle:
			return (flags & ML_DONTPEGBOTTOM) ? near_s->floor_height + tex_h : near_s->ceiling_height;
		case WallPart_Upper:
			return (flags & ML_DONTPEGTOP) ? near_s->ceiling_height : far_s->ceiling_height + tex_h;
		case WallPart_Lower:
			return (flags & ML_DONTPEGBOTTOM) ? near_s->ceiling_height : far_s->floor_height;
	}
	return 0.0f;
}

static void
add_wall_segment(Wall_Segment_List *list, Wall_Part part, u32 texture_number,
                 Line_Def *line, Side_Def *side, Sector *near_s, Sector *far_s,
                 V2 p1, V2 p2, f32 bottom_z, f32 top_z, Wall_Build_Context *context)
{
    if(texture_number == 0) { return; } // NOTE(Fermin): "-" means no texture

    Wall_Texture *tex = get_or_load_wall_texture(context->wad, context->cache, texture_number,
                                         context->platform_API, context->scratch);
    f32 tex_h = (f32)tex->height;
    f32 anchor = wall_anchor_top(part, line->flags, near_s, far_s, tex_h) + side->y_offset;

    Wall_Segment *seg = list->segments + list->segment_count++;
    seg->part = part;
    seg->texture_number = texture_number;
    seg->mesh = build_wall_quad(p1, p2, bottom_z, top_z, near_s->light_level,
                                side->x_offset, anchor, (f32)tex->width, tex_h, context->scratch);
    seg->mesh.texture_handle = tex->id;
}

static void
add_masked_middle_segment(Wall_Segment_List *list, Line_Def *line, Side_Def *side,
						  Sector *near_s, Sector *far_s, V2 p1, V2 p2,
						  Wall_Build_Context *ctx)
{
	if(side->middle_texture == 0) { return; } // NOTE(Fermin): "-" means no texture

	Wall_Texture *tex = get_or_load_wall_texture(ctx->wad, ctx->cache, side->middle_texture,
									  ctx->platform_API, ctx->scratch);
	f32 tex_w = (f32)tex->width;
	f32 tex_h = (f32)tex->height;

	// NOTE(Fermin): the opening between the two sectors. Symmetric, so near/far order doesn't matter.
	f32 opening_bottom = max(near_s->floor_height,   far_s->floor_height);
	f32 opening_top    = min(near_s->ceiling_height, far_s->ceiling_height);

	// NOTE(Fermin): vanilla rule (R_RenderMaskedSegRange): DONTPEGBOTTOM puts the texture
	// bottom on the higher floor, otherwise it hangs from the lower ceiling.
	f32 anchor = (line->flags & ML_DONTPEGBOTTOM) ? opening_bottom + tex_h : opening_top;
	anchor += side->y_offset;

	// NOTE(Fermin): drawn once (no vertical tiling), clipped to the opening.
	f32 top_z    = min(opening_top,    anchor);
	f32 bottom_z = max(opening_bottom, anchor - tex_h);
	if(top_z <= bottom_z) { return; }

	Wall_Segment *seg = list->segments + list->segment_count++;
	seg->part = WallPart_Masked_Middle;
	seg->texture_number = side->middle_texture;
	seg->mesh = build_wall_quad(p1, p2, bottom_z, top_z, near_s->light_level,
							 side->x_offset, anchor, tex_w, tex_h, ctx->scratch);
	seg->mesh.texture_handle = tex->id;
}

static Wall_Segment_List
build_wall_segments_for_line(Line_Def *line, V2 *vertex_positions, Wall_Build_Context *context)
{
	// NOTE(Fermin): 4 max -- at most upper + lower for a two-sided line + one masked middle PER SIDE
	Wall_Segment_List result = {};
	result.segments = push_array(context->scratch, 4, Wall_Segment); 

    V2 p1 = vertex_positions[line->v1];
    V2 p2 = vertex_positions[line->v2];
    Sector *front = line->front_sector;
    Sector *back  = line->back_sector;
    Side_Def *fs  = line->front_side;
    Side_Def *bs  = line->back_side;
    u32 sky = context->cache->sky_flat_number;

    if(!back)
    {
		// NOTE(Fermin): one-sided -- sidenum[1] == -1 in the WAD, solid floor-to-ceiling wall
        add_wall_segment(&result, WallPart_Middle, fs->middle_texture, line, fs, front, 0,
                         p1, p2, front->floor_height, front->ceiling_height, context);
        return result;
    }

    if(front->ceiling_height > back->ceiling_height &&
       !(front->ceiling_flat == sky && back->ceiling_flat == sky))
    {
		// NOTE(Fermin): ceiling drops going from front sector into back sector.
		// Skips it when both ceilings are sky.
        add_wall_segment(&result, WallPart_Upper, fs->upper_texture, line, fs, front, back,
                         p1, p2, back->ceiling_height, front->ceiling_height, context);
    }

    if(front->floor_height > back->floor_height)
    {
		// NOTE(Fermin): Floor drops going from front sector into back sector.
		// We build the quad facing the back sector, hence the inverted p
        add_wall_segment(&result, WallPart_Lower, bs->lower_texture, line, bs, back, front,
                         p2, p1, back->floor_height, front->floor_height, context);
    }

    if(back->floor_height > front->floor_height)
    {
		// NOTE(Fermin): floor rises going from front sector into back sector
        add_wall_segment(&result, WallPart_Lower, fs->lower_texture, line, fs, front, back,
                         p1, p2, front->floor_height, back->floor_height, context);
    }

    if(back->ceiling_height > front->ceiling_height &&
       !(front->ceiling_flat == sky && back->ceiling_flat == sky))
    {
		// NOTE(Fermin): Ceiling rises going from front sector into back sector
		// We build the quad facing the back sector, hence the inverted p
        add_wall_segment(&result, WallPart_Upper, bs->upper_texture, line, bs, back, front,
                         p2, p1, front->ceiling_height, back->ceiling_height, context);
    }

	add_masked_middle_segment(&result, line, fs, front, back, p1, p2, context);
	add_masked_middle_segment(&result, line, bs, back, front, p2, p1, context);

    return result;
}

static void
build_sector_render_data(Sector *sector, V2 *vertex_positions, Memory_Arena *scratch,
						 Platform_API *platform_API, Wad_File *wad, Texture_Cache *texture_cache,
						 u32 sector_index, i32 map_marker)
{
	/* 
	* The front of a Line_Def is always 90 degrees to the right / clockwise from the ray you
	* would draw starting from the first point to the second point.
	*/

	if(sector->line_count == 0) { return; }  // NOTE(Fermin): orphan sector, nothing to triangulate

	u32 patch_count = count_edge_patches(wad, map_marker, sector_index);
	Sector_Edge *edges = push_array(scratch, sector->line_count + patch_count, Sector_Edge);
	u32 edge_count = 0;
	for(u32 i = 0; i < sector->line_count; ++i)
	{
		Line_Def *line = sector->lines[i];
		if(line->front_sector == line->back_sector) { continue; } // same sector on both sides -> not part of the boundary
		if(line->front_sector == sector) { edges[edge_count++] = { line->v1, line->v2 }; }
		else                             { edges[edge_count++] = { line->v2, line->v1 }; }
	}

	for(u32 i = 0; i < array_count(edge_patches); ++i)
	{
		Edge_Patch *patch = edge_patches + i;
		if(!edge_patch_applies(patch, wad, map_marker, sector_index)) { continue; }

		edges[edge_count++] = { patch->from_vertex, patch->to_vertex };
		printf("PATCHED level %s, sector %u: added edge %u -> %u\n",
		 wad->lumps[map_marker].name, sector_index, patch->from_vertex, patch->to_vertex);
	}

#if 0
	// TODO(Fermin): Control through cmd params?
	debug_check_edge_balance(edges, edge_count, vertex_positions, sector_index, scratch);
#endif

	Chained_Loops chained_loops = chain_edges_to_loops(edges, edge_count, vertex_positions, scratch);

	Classified_Sector_Loops	classified_loops = classify_loops(&chained_loops, vertex_positions, scratch);

	for(u32 o = 0; o < classified_loops.outer_count; ++o)
	{
		Edge_Loop merged_loop = classified_loops.outer[o];
		for(u32 h = 0; h < merged_loop.hole_count; ++h)
		{
			merged_loop = merge_hole_into_outer(&merged_loop,
											    classified_loops.holes + merged_loop.hole_indices[h],
												vertex_positions, scratch);
		}

		Triangulated_Loop floor_triangles = triangulate_ear_clip(merged_loop.vertices,
															     merged_loop.vertex_count,
																 vertex_positions, scratch);

		Mesh floor_mesh = build_flat_mesh(&floor_triangles, vertex_positions,
										 sector->floor_height, sector->light_level,
										 true, scratch);
		floor_mesh.texture_handle = get_or_load_flat_id(wad, texture_cache, sector->floor_flat, platform_API);
		platform_API->upload_static_mesh_to_gpu(&floor_mesh);

		if(sector->ceiling_flat == texture_cache->sky_flat_number) { continue; } // temp

		Mesh ceiling_mesh = build_flat_mesh(&floor_triangles, vertex_positions,
										 sector->ceiling_height, sector->light_level,
										 false, scratch);
		ceiling_mesh.texture_handle = get_or_load_flat_id(wad, texture_cache, sector->ceiling_flat, platform_API);
		platform_API->upload_static_mesh_to_gpu(&ceiling_mesh);
	}
}

static void
group_lines_into_sectors(Line_Def *lines, u32 line_count, Sector *sectors, u32 sector_count,
						 Memory_Arena *arena, Memory_Arena *scratch)
{
	u32 *counts = push_array(scratch, sector_count, u32);
	for(u32 i = 0; i < line_count; ++i)
	{
		// How many lines reference each sector -- Sector 0 -> 2 lines.
		// This is so we can allocate exact memory below
		counts[lines[i].front_sector - sectors]++;
		if(lines[i].back_sector && lines[i].back_sector != lines[i].front_sector)
		{
			counts[lines[i].back_sector - sectors]++;
		}
	}

	for(u32 s = 0; s < sector_count; ++s)
	{
		// allocate buffer for lines in each sector
		sectors[s].lines = push_array(arena, counts[s], Line_Def *);
		sectors[s].line_count = 0; // NOTE(Fermin): refilled below, counts[] was just for sizing
	}

	for(u32 i = 0; i < line_count; ++i)
	{
		// populate sector's array with lines
		Sector *fs = lines[i].front_sector;
		fs->lines[fs->line_count++] = &lines[i];
		if(lines[i].back_sector && lines[i].back_sector != lines[i].front_sector)
		{
			Sector *bs = lines[i].back_sector;
			bs->lines[bs->line_count++] = &lines[i];
		}
	}
}
