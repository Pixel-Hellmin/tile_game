#if !defined(MATH_CPP)

union V2
{
    struct
    {
        f32 x, y;
    };
    struct
    {
        f32 u, v;
    };
    f32 e[2];
};

union V3
{
    struct
    {
        f32 x, y, z;
    };
    struct
    {
        f32 u, v, w;
    };
    struct
    {
        f32 r, g, b;
    };
    struct
    {
        V2 xy;
        f32 Ignored0_;
    };
    struct
    {
        f32 Ignored1_;
        V2 yz;
    };
    struct
    {
        V2 uv;
        f32 Ignored2_;
    };
    struct
    {
        f32 Ignored3_;
        V2 vw;
    };
    f32 e[3];
};

union V4
{
    struct
    {
        union
        {
            V3 xyz;
            struct
            {
                f32 x, y, z;
            };
        };
        
        f32 w;        
    };
    struct
    {
        union
        {
            V3 rgb;
            struct
            {
                f32 r, g, b;
            };
        };
        
        f32 a;        
    };
    struct
    {
        V2 xy;
        f32 Ignored0_;
        f32 Ignored1_;
    };
    struct
    {
        f32 Ignored2_;
        V2 yz;
        f32 Ignored3_;
    };
    struct
    {
        f32 Ignored4_;
        f32 Ignored5_;
        V2 zw;
    };
    f32 e[4];
};

union M4
{
    // NOTE(Fermin): OpenGL convention is column-major, my current
    // convention is row-major, I'll keep working
    // with this for now but may need to change.
    
    // NOTE(Fermin): Not sure how I'd like to store matrices.
    // I'll start with this and see how it goes. Additionally we can
    // use:
    // {
    //  V4 col/row1;
    //  V4 col/row2;
    //  V4 col/row3;
    //  V4 col/row4;
    // }

    // NOTE(Fermin): ROW then COLUMN
    struct
    {
        V4 m[4];
    };
    f32 e[16];
};

static inline f32 degrees(f32 radians)
{
    f32 result;
    result = radians * (180.0f / Pi32);

    return result;
}

static inline f32 radians(f32 degrees)
{
    f32 result;
    result = degrees * (Pi32 / 180.0f);

    return result;
}

static inline M4 m4_ident()
{
    M4 result = {};
    result.m[0].x = 1.0f;
    result.m[1].y = 1.0f;
    result.m[2].z = 1.0f;
    result.m[3].w = 1.0f;

    return result;
}

static inline M4 perspective(f32 fov_radians, f32 aspect_ratio, f32 z_near, f32 z_far)
{
    // NOTE(Fermin): This is a row-major perspective matrix

    M4 result = {};

    f32 f = 1.0f / tanf(fov_radians / 2.0f);

    result.m[0].x = f / aspect_ratio;
    result.m[1].y = f;
    result.m[2].z = (z_far + z_near) / (z_near - z_far);
    result.m[2].w = (2.0f * z_far * z_near) / (z_near - z_far);
    result.m[3].z = -1.0f;

    return result;
}

// NOTE(Fermin): From right to left order of operations for m4 transforms:
// scale -> rotate -> translate
// M4 transform = translate * rotate * scale;
static M4 scale_m4(V3 scale)
{
    // TODO(Fermin): Make the function name 'scale' work

    M4 result = m4_ident();

    result.m[0].x = scale.x;
    result.m[1].y = scale.y;
    result.m[2].z = scale.z;

    return result;
}

static M4 rotate(f32 radians, V3 axis)
{
    // NOTE(Fermin): angle must be a unit vector

    f32 cos_theta = cosf(radians);
    f32 sin_theta = sinf(radians);
    f32 one_minus_cos_theta = 1.0f - cos_theta;

    f32 x = axis.x;
    f32 y = axis.y;
    f32 z = axis.z;

    M4 result = m4_ident();

    result.m[0].x = cos_theta + x * x * one_minus_cos_theta;
    result.m[0].y = x * y * one_minus_cos_theta - z * sin_theta;
    result.m[0].z = x * z * one_minus_cos_theta + y * sin_theta;

    result.m[1].x = y * x * one_minus_cos_theta + z * sin_theta;
    result.m[1].y = cos_theta + y * y * one_minus_cos_theta;
    result.m[1].z = y * z * one_minus_cos_theta - x * sin_theta;

    result.m[2].x = z * x * one_minus_cos_theta - y * sin_theta;
    result.m[2].y = z * y * one_minus_cos_theta + x * sin_theta;
    result.m[2].z = cos_theta + z * z * one_minus_cos_theta;

    return result;
}

static M4 translate(V3 translation)
{
    M4 result = m4_ident();

    result.m[0].w = translation.x;
    result.m[1].w = translation.y;
    result.m[2].w = translation.z;

    return result;
}

static M4 multiply_m4(const M4* a, const M4* b)
{

    M4 result = {};

    for (i32 row = 0; row < 4; ++row) {
        for (i32 col = 0; col < 4; ++col) {
            result.m[row].e[col] = a->m[row].e[0] * b->m[0].e[col] +
                                   a->m[row].e[1] * b->m[1].e[col] +
                                   a->m[row].e[2] * b->m[2].e[col] +
                                   a->m[row].e[3] * b->m[3].e[col];
        }
    }

    return result;
}

static V4 multiply_m4_v4(const M4 *m, const V4 *v)
{

    V4 result = {};

    for (i32 row = 0; row < 4; ++row) {
        result.e[row] = m->m[row].e[0] * v->e[0] +
                        m->m[row].e[1] * v->e[1] +
                        m->m[row].e[2] * v->e[2] +
                        m->m[row].e[3] * v->e[3];
    }

    return result;
}

inline M4 operator*(M4 a, M4 b)
{
    M4 result = multiply_m4(&a, &b);

    return result;
}

inline V4 operator*(M4 a, V4 b)
{
    V4 result = multiply_m4_v4(&a, &b);

    return result;
}

#define MATH_CPP
#endif
