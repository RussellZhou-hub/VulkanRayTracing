struct Renderbuffer {
    int w, h, ys;
    void* data;
};

struct Vert {
    vec4f position;
    vec4f texcoord;
    vec4f color;
};

struct Varying {
    vec4f texcoord;
    vec4f color;
};

void vertex_shader(const Vert& in, vec4f& gl_Position, Varying& out)
{
    out.texcoord = in.texcoord;
    out.color = in.color;
    gl_Position = { in.position[0], in.position[1], -2 * in.position[2] - 2 * in.position[3], -in.position[2] };
}

void fragment_shader(vec4f& gl_FragCoord, const Varying& in, vec4f& out)
{
    out = in.color;
    vec2f wrapped = vec2f(in.texcoord - floor(in.texcoord));
    bool brighter = (wrapped[0] < 0.5) != (wrapped[1] < 0.5);
    if (!brighter)
        (vec3f&)out = 0.5f * (vec3f&)out;
}

void store_color(Renderbuffer& buf, int x, int y, const vec4f& c)
{
    // can do alpha composition here
    uint8_t* p = (uint8_t*)buf.data + buf.ys * (buf.h - y - 1) + 4 * x;
    p[0] = linear_to_srgb8(c[0]);
    p[1] = linear_to_srgb8(c[1]);
    p[2] = linear_to_srgb8(c[2]);
    p[3] = lround(c[3] * 255);
}

void draw_triangle(Renderbuffer& color_attachment, const box2f& viewport, const Vert* verts)
{
    Varying perVertex[3];
    vec4f gl_Position[3];

    box2f aabbf = { viewport.hi, viewport.lo };
    for (int i = 0; i < 3; ++i)
    {
        // invoke the vertex shader
        vertex_shader(verts[i], gl_Position[i], perVertex[i]);

        // convert to device coordinates by perspective division
        gl_Position[i][3] = 1 / gl_Position[i][3];
        gl_Position[i][0] *= gl_Position[i][3];
        gl_Position[i][1] *= gl_Position[i][3];
        gl_Position[i][2] *= gl_Position[i][3];

        // convert to window coordinates
        auto& pos2 = (vec2f&)gl_Position[i];
        pos2 = mix(viewport.lo, viewport.hi, 0.5f * (pos2 + vec2f(1)));
        aabbf = join(aabbf, (const vec2f&)gl_Position[i]);
    }

    // precompute the affine transform from fragment coordinates to barycentric coordinates
    const float denom = 1 / ((gl_Position[0][0] - gl_Position[2][0]) * (gl_Position[1][1] - gl_Position[0][1]) - (gl_Position[0][0] - gl_Position[1][0]) * (gl_Position[2][1] - gl_Position[0][1]));
    const vec3f barycentric_d0 = denom * vec3f(gl_Position[1][1] - gl_Position[2][1], gl_Position[2][1] - gl_Position[0][1], gl_Position[0][1] - gl_Position[1][1]);
    const vec3f barycentric_d1 = denom * vec3f(gl_Position[2][0] - gl_Position[1][0], gl_Position[0][0] - gl_Position[2][0], gl_Position[1][0] - gl_Position[0][0]);
    const vec3f barycentric_0 = denom * vec3f(
        gl_Position[1][0] * gl_Position[2][1] - gl_Position[2][0] * gl_Position[1][1],
        gl_Position[2][0] * gl_Position[0][1] - gl_Position[0][0] * gl_Position[2][1],
        gl_Position[0][0] * gl_Position[1][1] - gl_Position[1][0] * gl_Position[0][1]
    );

    // loop over all pixels in the rectangle bounding the triangle
    const box2i aabb = lround(aabbf);
    for (int y = aabb.lo[1]; y < aabb.hi[1]; ++y)
        for (int x = aabb.lo[0]; x < aabb.hi[0]; ++x)
        {
            vec4f gl_FragCoord;
            gl_FragCoord[0] = x + 0.5;
            gl_FragCoord[1] = y + 0.5;

            // fragment barycentric coordinates in window coordinates
            const vec3f barycentric = gl_FragCoord[0] * barycentric_d0 + gl_FragCoord[1] * barycentric_d1 + barycentric_0;

            // discard fragment outside the triangle. this doesn't handle edges correctly.
            if (barycentric[0] < 0 || barycentric[1] < 0 || barycentric[2] < 0)
                continue;

            // interpolate inverse depth linearly
            gl_FragCoord[2] = dot(barycentric, vec3f(gl_Position[0][2], gl_Position[1][2], gl_Position[2][2]));
            gl_FragCoord[3] = dot(barycentric, vec3f(gl_Position[0][3], gl_Position[1][3], gl_Position[2][3]));

            // clip fragments to the near/far planes (as if by GL_ZERO_TO_ONE)
            if (gl_FragCoord[2] < 0 || gl_FragCoord[2] > 1)
                continue;

            // convert to perspective correct (clip-space) barycentric
            const vec3f perspective = 1 / gl_FragCoord[3] * barycentric * vec3f(gl_Position[0][3], gl_Position[1][3], gl_Position[2][3]);

            // interpolate the attributes using the perspective correct barycentric
            Varying varying;
            for (int i = 0; i < sizeof(Varying) / sizeof(float); ++i)
                ((float*)&varying)[i] = dot(perspective, vec3f(
                    ((const float*)&perVertex[0])[i],
                    ((const float*)&perVertex[1])[i],
                    ((const float*)&perVertex[2])[i]
                ));

            // invoke the fragment shader and store the result
            vec4f color;
            fragment_shader(gl_FragCoord, varying, color);
            store_color(color_attachment, x, y, color);
        }
}

int main()
{
    Renderbuffer buffer = { 512, 512, 512 * 4 };
    buffer.data = calloc(buffer.ys, buffer.h);

    // interleaved attributes buffer
    Vert verts[] = {
        { { -1, -1, -2, 1 }, { 0, 0, 0, 1 }, { 0, 0, 1, 1 } },
        { { 1, -1, -1, 1 }, { 10, 0, 0, 1 }, { 1, 0, 0, 1 } },
        { { 0, 1, -1, 1 }, { 0, 10, 0, 1 }, { 0, 1, 0, 1 } },
    };

    box2f viewport = { 0, 0, buffer.w, buffer.h };
    draw_triangle(buffer, viewport, verts);

    stbi_write_png("out.png", buffer.w, buffer.h, 4, buffer.data, buffer.ys);
}