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
    Edge_Loop *loops = push_array(arena, edge_count, Edge_Loop);

	// NOTE(Fermin): Walk each unvisited edge until it closes
    for(u32 start_edge_index = 0; start_edge_index < edge_count; ++start_edge_index)
    {
        if(visited[start_edge_index]) { continue; }

        Edge_Loop *loop = loops + result.loop_count++;
        *loop = {};
		// NOTE(Femin): upper bound, trim later if you care.
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
    f32 area = 0.0f;
    for(u32 i = 0; i < loop->vertex_count; ++i)
    {
        V2 a = vertex_positions[loop->vertices[i]];
        V2 b = vertex_positions[loop->vertices[(i + 1) % loop->vertex_count]];
        area += (a.x * b.y) - (b.x * a.y);
    }
    return 0.5f * area;
}

static Classified_Sector_Loops
classify_loops(Chained_Loops *chained, V2 *vertex_positions, Memory_Arena *arena)
{
    Classified_Sector_Loops result = {};
    result.holes = push_array(arena, chained->loop_count, Edge_Loop);

    for(u32 loop_index = 0; loop_index < chained->loop_count; ++loop_index)
    {
        Edge_Loop *loop = chained->loops + loop_index;
        loop->signed_area = signed_area_for_loop(loop, vertex_positions);

        if(loop->signed_area > 0.0f)
        {
            // NOTE(Fermin): convention -> positive area is the outer boundary
            assert(!result.outer); // NOTE(Fermin): a well-formed sector has exactly one outer loop
            result.outer = loop;
        }
        else
        {
            result.holes[result.hole_count++] = *loop;
        }
    }

    return result;
}
