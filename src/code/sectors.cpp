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
    f32 signed_area;    // filled in by classify_loops
};

struct Chained_Loops
{
    Edge_Loop *loops;
    u32 loop_count;
};

struct Classified_Sector_Loops
{
    Edge_Loop *outer;
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

struct Mesh_Vertex
{
    V3 position; // in the world
    V2 uv;
    f32 light;   // flat per-sector light
};

struct Mesh
{
	Mesh_Vertex *vertices;
	u32 vertex_count;
	u32 *indices; // @Cleanup: Whats the point of these?
	u32 index_count;
};

#define INVALID_EDGE_INDEX 0xFFFFFFFF
static Chained_Loops
chain_edges_to_loops(Sector_Edge *edges, u32 edge_count, Memory_Arena *arena)
{
    Chained_Loops result = {};

    // NOTE(Fermin): find the vertex index range so edge_by_start can be a flat array
    u32 max_vertex_index = 0;
    for(u32 edge_index = 0; edge_index < edge_count; ++edge_index)
    {
        Sector_Edge *edge = edges + edge_index;
        if(edge->v1 > max_vertex_index) { max_vertex_index = edge->v1; }
        if(edge->v2 > max_vertex_index) { max_vertex_index = edge->v2; }
    }

    u32 *edge_by_start = push_array(arena, max_vertex_index + 1, u32);
    for(u32 vertex_index = 0; vertex_index <= max_vertex_index; ++vertex_index)
    {
        edge_by_start[vertex_index] = INVALID_EDGE_INDEX;
    }
    for(u32 edge_index = 0; edge_index < edge_count; ++edge_index)
    {
		// table that says: At vertex X we got this edge index
		// given the vertex you just arrived at, one array lookup tells you the next edge
        edge_by_start[edges[edge_index].v1] = edge_index;
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
		// NOTE(Fermin): upper bound, trim later if you care.
		// vertices number is edge_count cause v2 of e0 == v1 of e1
        loop->vertices = push_array(arena, edge_count, u32); 

        u32 start_vertex = edges[start_edge_index].v1;
        u32 current_edge_index = start_edge_index;

        for(;;)
        {
            assert(!visited[current_edge_index]);
            visited[current_edge_index] = true;

            Sector_Edge *current_edge = edges + current_edge_index;
            loop->vertices[loop->vertex_count++] = current_edge->v1;

            if(current_edge->v2 == start_vertex) { break; }

            u32 next_edge_index = edge_by_start[current_edge->v2];
            assert(next_edge_index != INVALID_EDGE_INDEX); // NOTE(Fermin): open chain -> malformed sector data
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

static Classified_Sector_Loops
classify_loops(Chained_Loops *chained, V2 *vertex_positions, Memory_Arena *arena)
{
    Classified_Sector_Loops result = {};
	// there is only one outer loop per sector
	assert(chained->loop_count > 0); // every sector has at least one loop (the outer boundary)
    result.holes = push_array(arena, (chained->loop_count - 1), Edge_Loop); 

    for(u32 loop_index = 0; loop_index < chained->loop_count; ++loop_index)
    {
        Edge_Loop *loop = chained->loops + loop_index;
        loop->signed_area = signed_area_for_loop(loop, vertex_positions);

        if(loop->signed_area > 0.0f)
        {
            // NOTE(Fermin): convention -> positive area is the outer boundary
            assert(!result.outer); // a well-formed sector has exactly one outer loop
            result.outer = loop;
        }
        else
        {
            result.holes[result.hole_count++] = *loop;
        }
    }

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

	// is it convex?
    if(cross((b - a), (c - b)) <= 0.0f) { return false; }

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
build_flat_mesh(Triangulated_Loop *tri, V2 *vertex_positions, f32 z, f32 light, b32 flip_winding, Memory_Arena *arena)
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

struct GPU_Mesh // move to opengl
{
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    u32 index_count;
};

static GPU_Mesh // move to opengl
upload_mesh_to_gpu(Mesh *mesh)
{
	GPU_Mesh result = {};
	result.index_count = mesh->index_count;

	opengl.glGenVertexArrays(1, &result.vao);
	opengl.glGenBuffers(1, &result.vbo);
	opengl.glGenBuffers(1, &result.ebo);

	opengl.glBindVertexArray(result.vao);

	opengl.glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
	// GL_STATIC_DRAW because sector geometry never changes at runtime -- upload once, draw every frame
	opengl.glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count * sizeof(Mesh_Vertex), mesh->vertices, GL_STATIC_DRAW);

	opengl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
	opengl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->index_count * sizeof(u32), mesh->indices, GL_STATIC_DRAW);

	opengl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void *)offsetof(Mesh_Vertex, position));
	opengl.glEnableVertexAttribArray(0);

	opengl.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void *)offsetof(Mesh_Vertex, uv));
	opengl.glEnableVertexAttribArray(1);

	opengl.glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void *)offsetof(Mesh_Vertex, light));
	opengl.glEnableVertexAttribArray(2);

	opengl.glBindVertexArray(0); // unbind so later calls don't accidentally clobber this VAO's state
	return result;
}

static void // move to opengl
draw_gpu_mesh(GPU_Mesh *mesh)
{
	opengl.glBindVertexArray(mesh->vao);
	glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
	opengl.glBindVertexArray(0);
}

static void
debug_generate_geometry(V2 *vertex_positions, Sector_Edge *edges, u32 edge_count, Memory_Arena *debug_arena)
{
	/*
	* From floor/ceiling vertices and edges to meshes
	* @Cleanup: Use scratch arena for all intermediate steps
	* and save only the data we need
	*/

	Chained_Loops chained_loops = chain_edges_to_loops(edges, edge_count, debug_arena);
	Classified_Sector_Loops	classified_loops = classify_loops(&chained_loops, vertex_positions, debug_arena);
	// NOTE(Fermin): For multiple holes call this again with the
	// previous result as the new outer
	assert(classified_loops.hole_count == 1);
	Edge_Loop merged_loop = merge_hole_into_outer(classified_loops.outer, classified_loops.holes, vertex_positions, debug_arena);
	Triangulated_Loop triangles = triangulate_ear_clip(merged_loop.vertices,
													   merged_loop.vertex_count,
													   vertex_positions,
													   debug_arena);
	f32 sector_light_level = 1.0f;
	f32 sector_floor_height = 0.0f;
	f32 sector_ceiling_height = 256.0f;
	Mesh floor_mesh   = build_flat_mesh(&triangles, vertex_positions,
									    sector_floor_height,
									    sector_light_level / 255.0f,
									    false,
										debug_arena);
	Mesh ceiling_mesh = build_flat_mesh(&triangles, vertex_positions,
										sector_ceiling_height,
										sector_light_level / 255.0f,
										true,
										debug_arena);
}
