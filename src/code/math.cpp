#if !defined(MATH_CPP)
// NOTE(Fermin): This is the start of the code that should go in an intrinsics file
static inline f32 square_root(f32 a)
{
    f32 result = sqrtf(a);
    return result;
}
// NOTE(Fermin): THis is the end of the intrinsics file
inline f32 lerp(f32 a, f32 t, f32 b)
{
    f32 result = (1.0f - t) * a + t * b;

    return result;
}

static inline f32 clamp(f32 min, f32 value, f32 max)
{
    f32 result = value;
    
    if(result < min)
    {
        result = min;
    }
    else if(result > max)
    {
        result = max;
    }

    return result;
}

static inline f32 clamp01(f32 value)
{
    f32 result = clamp(0.0f, value, 1.0f);

    return result;
}

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

/*
* V2
*/
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

inline V2 operator+(V2 a, V2 b)
{
    V2 result = {};

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

inline V2 operator-(V2 a, V2 b)
{
    V2 result = {};

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

inline V2 & operator+=(V2 &a, V2 b)
{
    a = a + b;

    return a;
}

inline V2 & operator-=(V2 &a, V2 b)
{
    a = a - b;

    return a;
}

/*
* V3
*/
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

inline V3 operator+(V3 a, V3 b)
{
    V3 result = {};

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

inline V3 operator-(V3 a, V3 b)
{
    V3 result = {};

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

inline V3 & operator+=(V3 &a, V3 b)
{
    a = a + b;

    return a;
}

inline V3 & operator-=(V3 &a, V3 b)
{
    a = a - b;

    return a;
}

inline V3 operator*(f32 b, V3 a)
{
    V3 result = {};

    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;

    return result;
}

inline V3 operator*(V3 a, f32 b)
{
    V3 result = {};

    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;

    return result;
}

inline V3 & operator*=(V3 &a, f32 b)
{
    a = a * b;

    return a;
}

inline V3 operator/(V3 a, f32 b)
{
    V3 result = {};

    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;

    return result;
}

static inline f32 inner(V3 a, V3 b)
{
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z;

    return result;
}

static inline V3 cross(V3 a, V3 b)
{
    V3 result = {};

    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;

    return result;
}

static inline f32 length_sq(V3 a)
{
    f32 result = inner(a, a);

    return result;
}

static inline f32 length(V3 a)
{
    f32 result = square_root(length_sq(a));

    return result;
}

static inline V3 normalize(V3 a)
{
    V3 result = a * (1.0f / length(a));

    return result;
}

/*
* V4
*/
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

inline V4 operator*(f32 b, V4 a)
{
    V4 result = {};

    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    result.w = a.w * b;

    return result;
}

inline V4 operator*(V4 a, f32 b)
{
    V4 result = {};

    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    result.w = a.w * b;

    return result;
}

inline V4 operator+(V4 a, V4 b)
{
    V4 result = {};

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;

    return result;
}

inline V4 & operator+=(V4 &a, V4 b)
{
    a = a + b;

    return a;
}

/*
* M4
*/
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

static inline M4 multiply_m4(const M4* a, const M4* b)
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

static inline V4 multiply_m4_v4(const M4 *m, const V4 *v)
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

static inline V4 operator*(M4 a, V4 b)
{
    V4 result = multiply_m4_v4(&a, &b);

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

static inline M4 orthogonal(f32 left, f32 right, f32 bottom, f32 top, f32 z_near, f32 z_far)
{
    // NOTE: This is a column-major orthographic matrix

    M4 result = {};

    result.m[0].x = 2.0f / (right - left);
    result.m[1].y = 2.0f / (top - bottom);
    result.m[2].z = -2.0f / (z_far - z_near);
    result.m[3].w = 1.0f;

    result.m[3].x = -(right + left) / (right - left);
    result.m[3].y = -(top + bottom) / (top - bottom);
    result.m[3].z = -(z_far + z_near) / (z_far - z_near);

    /*
    result.m[0].w = -(right + left) / (right - left);
    result.m[1].w = -(top + bottom) / (top - bottom);
    result.m[2].w = -(z_far + z_near) / (z_far - z_near);
    */

    return result;
}
// NOTE(Fermin): From right to left order of operations for m4 transforms:
// scale -> rotate -> translate
// M4 transform = translate * rotate * scale;
static inline M4 scale_m4(V3 scale)
{
    // TODO(Fermin): Make the function name 'scale' work

    M4 result = m4_ident();

    result.m[0].x = scale.x;
    result.m[1].y = scale.y;
    result.m[2].z = scale.z;

    return result;
}

static inline M4 rotate(f32 radians, V3 axis)
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

static inline M4 translate(V3 translation)
{
    M4 result = m4_ident();

    result.m[0].w = translation.x;
    result.m[1].w = translation.y;
    result.m[2].w = translation.z;

    return result;
}

static inline M4 look_at(V3 pos, V3 target, V3 up)
{
    // NOTE(Fermin): For the view matrix's coordinate system we want its z-axis to be positive and because by convention (in OpenGL) the camera points towards the negative z-axis we want to negate the direction vector. If we switch the subtraction order around we now get a vector pointing towards the camera's positive z-axis:
    V3 camera_direction = normalize(pos - target);
    V3 camera_right = normalize(cross(up, camera_direction));
    V3 camera_up = cross(camera_direction, camera_right);

    M4 result = {};

    /* ROW MAJOR
    */
    result.m[0].x = camera_right.x;
    result.m[0].y = camera_right.y;
    result.m[0].z = camera_right.z;
    result.m[0].w = -inner(camera_right, pos);

    result.m[1].x = camera_up.x;
    result.m[1].y = camera_up.y;
    result.m[1].z = camera_up.z;
    result.m[1].w = -inner(camera_up, pos);

    result.m[2].x = camera_direction.x;
    result.m[2].y = camera_direction.y;
    result.m[2].z = camera_direction.z;
    result.m[2].w = -inner(camera_direction, pos);

    result.m[3].x = 0;
    result.m[3].y = 0;
    result.m[3].z = 0;
    result.m[3].w = 1;

    /* COLUMN MAJOR
    result.m[0].x = camera_right.x;
    result.m[0].y = camera_up.x;
    result.m[0].z = camera_direction.x;
    result.m[0].w = 0;

    result.m[1].x = camera_right.y;
    result.m[1].y = camera_up.y;
    result.m[1].z = camera_direction.y;
    result.m[1].w = 0;

    result.m[2].x = camera_right.z;
    result.m[2].y = camera_up.z;
    result.m[2].z = camera_direction.z;
    result.m[2].w = 0;

    result.m[3].x = -inner(camera_right, pos);
    result.m[3].y = -inner(camera_up, pos);
    result.m[3].z = -inner(camera_direction, pos);
    result.m[3].w = 1;
    */

    return result;
}

static inline f32 determinant_4x4(M4* mat)
{
    // NOTE(Fermin): Chatgpt says this is column major
    f32 det;
    det  = mat->m[0].e[3] * mat->m[1].e[2] * mat->m[2].e[1] * mat->m[3].e[0] - mat->m[0].e[2] * mat->m[1].e[3] * mat->m[2].e[1] * mat->m[3].e[0] -
           mat->m[0].e[3] * mat->m[1].e[1] * mat->m[2].e[2] * mat->m[3].e[0] + mat->m[0].e[1] * mat->m[1].e[3] * mat->m[2].e[2] * mat->m[3].e[0] +
           mat->m[0].e[2] * mat->m[1].e[1] * mat->m[2].e[3] * mat->m[3].e[0] - mat->m[0].e[1] * mat->m[1].e[2] * mat->m[2].e[3] * mat->m[3].e[0] -
           mat->m[0].e[3] * mat->m[1].e[2] * mat->m[2].e[0] * mat->m[3].e[1] + mat->m[0].e[2] * mat->m[1].e[3] * mat->m[2].e[0] * mat->m[3].e[1] +
           mat->m[0].e[3] * mat->m[1].e[0] * mat->m[2].e[2] * mat->m[3].e[1] - mat->m[0].e[0] * mat->m[1].e[3] * mat->m[2].e[2] * mat->m[3].e[1] -
           mat->m[0].e[2] * mat->m[1].e[0] * mat->m[2].e[3] * mat->m[3].e[1] + mat->m[0].e[0] * mat->m[1].e[2] * mat->m[2].e[3] * mat->m[3].e[1] +
           mat->m[0].e[3] * mat->m[1].e[1] * mat->m[2].e[0] * mat->m[3].e[2] - mat->m[0].e[1] * mat->m[1].e[3] * mat->m[2].e[0] * mat->m[3].e[2] -
           mat->m[0].e[3] * mat->m[1].e[0] * mat->m[2].e[1] * mat->m[3].e[2] + mat->m[0].e[0] * mat->m[1].e[3] * mat->m[2].e[1] * mat->m[3].e[2] +
           mat->m[0].e[1] * mat->m[1].e[0] * mat->m[2].e[3] * mat->m[3].e[2] - mat->m[0].e[0] * mat->m[1].e[1] * mat->m[2].e[3] * mat->m[3].e[2] -
           mat->m[0].e[2] * mat->m[1].e[1] * mat->m[2].e[0] * mat->m[3].e[3] + mat->m[0].e[1] * mat->m[1].e[2] * mat->m[2].e[0] * mat->m[3].e[3] +
           mat->m[0].e[2] * mat->m[1].e[0] * mat->m[2].e[1] * mat->m[3].e[3] - mat->m[0].e[0] * mat->m[1].e[2] * mat->m[2].e[1] * mat->m[3].e[3] -
           mat->m[0].e[1] * mat->m[1].e[0] * mat->m[2].e[2] * mat->m[3].e[3] + mat->m[0].e[0] * mat->m[1].e[1] * mat->m[2].e[2] * mat->m[3].e[3];

    return det;
}

/*
static inline f32 determinant_4x4(M4* mat)
{
    // NOTE(Fermin): Chatgpt says this is row major
    f32 det;
    det  = mat->m[3].e[0] * mat->m[2].e[1] * mat->m[1].e[2] * mat->m[0].e[3] - mat->m[2].e[0] * mat->m[3].e[1] * mat->m[1].e[2] * mat->m[0].e[3] -
           mat->m[3].e[0] * mat->m[1].e[1] * mat->m[2].e[2] * mat->m[0].e[3] + mat->m[1].e[0] * mat->m[3].e[1] * mat->m[2].e[2] * mat->m[0].e[3] +
           mat->m[2].e[0] * mat->m[1].e[1] * mat->m[3].e[2] * mat->m[0].e[3] - mat->m[1].e[0] * mat->m[2].e[1] * mat->m[3].e[2] * mat->m[0].e[3] -
           mat->m[3].e[0] * mat->m[2].e[1] * mat->m[0].e[2] * mat->m[1].e[3] + mat->m[2].e[0] * mat->m[3].e[1] * mat->m[0].e[2] * mat->m[1].e[3] +
           mat->m[3].e[0] * mat->m[0].e[1] * mat->m[2].e[2] * mat->m[1].e[3] - mat->m[0].e[0] * mat->m[3].e[1] * mat->m[2].e[2] * mat->m[1].e[3] -
           mat->m[2].e[0] * mat->m[0].e[1] * mat->m[3].e[2] * mat->m[1].e[3] + mat->m[0].e[0] * mat->m[2].e[1] * mat->m[3].e[2] * mat->m[1].e[3] +
           mat->m[3].e[0] * mat->m[1].e[1] * mat->m[0].e[2] * mat->m[2].e[3] - mat->m[1].e[0] * mat->m[3].e[1] * mat->m[0].e[2] * mat->m[2].e[3] -
           mat->m[3].e[0] * mat->m[0].e[1] * mat->m[1].e[2] * mat->m[2].e[3] + mat->m[0].e[0] * mat->m[3].e[1] * mat->m[1].e[2] * mat->m[2].e[3] +
           mat->m[1].e[0] * mat->m[0].e[1] * mat->m[3].e[2] * mat->m[2].e[3] - mat->m[0].e[0] * mat->m[1].e[1] * mat->m[3].e[2] * mat->m[2].e[3] -
           mat->m[2].e[0] * mat->m[1].e[1] * mat->m[0].e[2] * mat->m[3].e[3] + mat->m[1].e[0] * mat->m[2].e[1] * mat->m[0].e[2] * mat->m[3].e[3] +
           mat->m[2].e[0] * mat->m[0].e[1] * mat->m[1].e[2] * mat->m[3].e[3] - mat->m[0].e[0] * mat->m[2].e[1] * mat->m[1].e[2] * mat->m[3].e[3] -
           mat->m[1].e[0] * mat->m[0].e[1] * mat->m[2].e[2] * mat->m[3].e[3] + mat->m[0].e[0] * mat->m[1].e[1] * mat->m[2].e[2] * mat->m[3].e[3];

    return det;
}
*/

static inline M4 adjoint(M4* mat)
{
    M4 adj;

    adj.m[0].e[0] =  mat->m[1].e[1] * (mat->m[2].e[2] * mat->m[3].e[3] - mat->m[2].e[3] * mat->m[3].e[2]) - mat->m[1].e[2] * (mat->m[2].e[1] * mat->m[3].e[3] - mat->m[2].e[3] * mat->m[3].e[1]) + mat->m[1].e[3] * (mat->m[2].e[1] * mat->m[3].e[2] - mat->m[2].e[2] * mat->m[3].e[1]);
    adj.m[0].e[1] = -mat->m[0].e[1] * (mat->m[2].e[2] * mat->m[3].e[3] - mat->m[2].e[3] * mat->m[3].e[2]) + mat->m[0].e[2] * (mat->m[2].e[1] * mat->m[3].e[3] - mat->m[2].e[3] * mat->m[3].e[1]) - mat->m[0].e[3] * (mat->m[2].e[1] * mat->m[3].e[2] - mat->m[2].e[2] * mat->m[3].e[1]);
    adj.m[0].e[2] =  mat->m[0].e[1] * (mat->m[1].e[2] * mat->m[3].e[3] - mat->m[1].e[3] * mat->m[3].e[2]) - mat->m[0].e[2] * (mat->m[1].e[1] * mat->m[3].e[3] - mat->m[1].e[3] * mat->m[3].e[1]) + mat->m[0].e[3] * (mat->m[1].e[1] * mat->m[3].e[2] - mat->m[1].e[2] * mat->m[3].e[1]);
    adj.m[0].e[3] = -mat->m[0].e[1] * (mat->m[1].e[2] * mat->m[2].e[3] - mat->m[1].e[3] * mat->m[2].e[2]) + mat->m[0].e[2] * (mat->m[1].e[1] * mat->m[2].e[3] - mat->m[1].e[3] * mat->m[2].e[1]) - mat->m[0].e[3] * (mat->m[1].e[1] * mat->m[2].e[2] - mat->m[1].e[2] * mat->m[2].e[1]);

    adj.m[1].e[0] = -mat->m[1].e[0] * (mat->m[2].e[2] * mat->m[3].e[3] - mat->m[2].e[3] * mat->m[3].e[2]) + mat->m[1].e[2] * (mat->m[2].e[0] * mat->m[3].e[3] - mat->m[2].e[3] * mat->m[3].e[0]) - mat->m[1].e[3] * (mat->m[2].e[0] * mat->m[3].e[2] - mat->m[2].e[2] * mat->m[3].e[0]);
    adj.m[1].e[1] =  mat->m[0].e[0] * (mat->m[2].e[2] * mat->m[3].e[3] - mat->m[2].e[3] * mat->m[3].e[2]) - mat->m[0].e[2] * (mat->m[2].e[0] * mat->m[3].e[3] - mat->m[2].e[3] * mat->m[3].e[0]) + mat->m[0].e[3] * (mat->m[2].e[0] * mat->m[3].e[2] - mat->m[2].e[2] * mat->m[3].e[0]);
    adj.m[1].e[2] = -mat->m[0].e[0] * (mat->m[1].e[2] * mat->m[3].e[3] - mat->m[1].e[3] * mat->m[3].e[2]) + mat->m[0].e[2] * (mat->m[1].e[0] * mat->m[3].e[3] - mat->m[1].e[3] * mat->m[3].e[0]) - mat->m[0].e[3] * (mat->m[1].e[0] * mat->m[3].e[2] - mat->m[1].e[2] * mat->m[3].e[0]);
    adj.m[1].e[3] =  mat->m[0].e[0] * (mat->m[1].e[2] * mat->m[2].e[3] - mat->m[1].e[3] * mat->m[2].e[2]) - mat->m[0].e[2] * (mat->m[1].e[0] * mat->m[2].e[3] - mat->m[1].e[3] * mat->m[2].e[0]) + mat->m[0].e[3] * (mat->m[1].e[0] * mat->m[2].e[2] - mat->m[1].e[2] * mat->m[2].e[0]);

    adj.m[2].e[0] =  mat->m[1].e[0] * (mat->m[2].e[1] * mat->m[3].e[3] - mat->m[2].e[3] * mat->m[3].e[1]) - mat->m[1].e[1] * (mat->m[2].e[0] * mat->m[3].e[3] - mat->m[2].e[3] * mat->m[3].e[0]) + mat->m[1].e[3] * (mat->m[2].e[0] * mat->m[3].e[1] - mat->m[2].e[1] * mat->m[3].e[0]);
    adj.m[2].e[1] = -mat->m[0].e[0] * (mat->m[2].e[1] * mat->m[3].e[3] - mat->m[2].e[3] * mat->m[3].e[1]) + mat->m[0].e[1] * (mat->m[2].e[0] * mat->m[3].e[3] - mat->m[2].e[3] * mat->m[3].e[0]) - mat->m[0].e[3] * (mat->m[2].e[0] * mat->m[3].e[1] - mat->m[2].e[1] * mat->m[3].e[0]);
    adj.m[2].e[2] =  mat->m[0].e[0] * (mat->m[1].e[1] * mat->m[3].e[3] - mat->m[1].e[3] * mat->m[3].e[1]) - mat->m[0].e[1] * (mat->m[1].e[0] * mat->m[3].e[3] - mat->m[1].e[3] * mat->m[3].e[0]) + mat->m[0].e[3] * (mat->m[1].e[0] * mat->m[3].e[1] - mat->m[1].e[1] * mat->m[3].e[0]);
    adj.m[2].e[3] = -mat->m[0].e[0] * (mat->m[1].e[1] * mat->m[2].e[3] - mat->m[1].e[3] * mat->m[2].e[1]) + mat->m[0].e[1] * (mat->m[1].e[0] * mat->m[2].e[3] - mat->m[1].e[3] * mat->m[2].e[0]) - mat->m[0].e[3] * (mat->m[1].e[0] * mat->m[2].e[1] - mat->m[1].e[1] * mat->m[2].e[0]);

    adj.m[3].e[0] = -mat->m[1].e[0] * (mat->m[2].e[1] * mat->m[3].e[2] - mat->m[2].e[2] * mat->m[3].e[1]) + mat->m[1].e[1] * (mat->m[2].e[0] * mat->m[3].e[2] - mat->m[2].e[2] * mat->m[3].e[0]) - mat->m[1].e[2] * (mat->m[2].e[0] * mat->m[3].e[1] - mat->m[2].e[1] * mat->m[3].e[0]);
    adj.m[3].e[1] =  mat->m[0].e[0] * (mat->m[2].e[1] * mat->m[3].e[2] - mat->m[2].e[2] * mat->m[3].e[1]) - mat->m[0].e[1] * (mat->m[2].e[0] * mat->m[3].e[2] - mat->m[2].e[2] * mat->m[3].e[0]) + mat->m[0].e[2] * (mat->m[2].e[0] * mat->m[3].e[1] - mat->m[2].e[1] * mat->m[3].e[0]);
    adj.m[3].e[2] = -mat->m[0].e[0] * (mat->m[1].e[1] * mat->m[3].e[2] - mat->m[1].e[2] * mat->m[3].e[1]) + mat->m[0].e[1] * (mat->m[1].e[0] * mat->m[3].e[2] - mat->m[1].e[2] * mat->m[3].e[0]) - mat->m[0].e[2] * (mat->m[1].e[0] * mat->m[3].e[1] - mat->m[1].e[1] * mat->m[3].e[0]);
    adj.m[3].e[3] =  mat->m[0].e[0] * (mat->m[1].e[1] * mat->m[2].e[2] - mat->m[1].e[2] * mat->m[2].e[1]) - mat->m[0].e[1] * (mat->m[1].e[0] * mat->m[2].e[2] - mat->m[1].e[2] * mat->m[2].e[0]) + mat->m[0].e[2] * (mat->m[1].e[0] * mat->m[2].e[1] - mat->m[1].e[1] * mat->m[2].e[0]);

    return adj;
}

static inline M4 invert(M4* mat)
{
    f32 det = determinant_4x4(mat);
    if (det == 0.0f) {
        fprintf(stderr, "Matrix is singular and cannot be inverted.\n");
        exit(EXIT_FAILURE);
    }

    M4 adj = adjoint(mat);
    M4 inv;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            inv.m[i].e[j] = adj.m[i].e[j] / det;
        }
    }

    return inv;
}

#define MATH_CPP
#endif
