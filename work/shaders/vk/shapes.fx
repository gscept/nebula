//------------------------------------------------------------------------------
//  shapes.fx
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

// put variables in push-constant block
push constant ShapeConstants [ string Visibility = "VS|GS|PS"; ]
{
    mat4 ShapeModel;
    vec4 ShapeColor;
    float LineWidth;
};

render_state WireframeState
{
    CullMode = None;
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    //MultisampleEnabled = true;
    };

render_state DepthEnabledState
{
    CullMode = None;
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    DepthEnabled = true;
    DepthWrite = true;
    //MultisampleEnabled = true;
};

render_state DepthDisabledState
{
    CullMode = None;
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    DepthEnabled = false;
    DepthWrite = false;
    //MultisampleEnabled = true;
};
//------------------------------------------------------------------------------
/**
*/
shader
void
vsMesh(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    [slot=1] in vec3 normal,
    [slot=3] in vec3 tangent,
    out vec4 Color) 
{
    gl_Position = ViewProjection * ShapeConstants.ShapeModel * vec4(position, 1);
    Color = ShapeConstants.ShapeColor;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsPrimitives(
    [slot=0] in vec3 position,
    [slot=5] in vec4 color, 
    out vec4 Color) 
{
    gl_Position = ViewProjection * ShapeConstants.ShapeModel * vec4(position, 1);
    Color = color;
}

//------------------------------------------------------------------------------
/**
*/
[input_primitive] = lines
[output_primitive] = triangle_strip
[max_vertex_count] = 4
shader
void
gsLines(
    in vec4 colors[],
    out noperspective float EdgeDistance,
    out vec4 Color)
{
    // Unproject points
    vec2 p0 = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
    vec2 p1 = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;

    // This is the line between the previous point and the first point
    vec2 line0 = (p1 - p0);
    
    // Aspect correct the lines
    line0 = normalize(line0.yx * vec2(-RenderTargetDimensions[0].y * RenderTargetDimensions[0].z, 1));

    // Scale by line width and inversed resolution
    line0 *= RenderTargetDimensions[0].zw * ShapeConstants.LineWidth;

    // Emit triangle strip 
    Color = colors[0];
    EdgeDistance = -ShapeConstants.LineWidth;
    gl_Position = vec4((p0 - line0) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw);
    EmitVertex();

    Color = colors[0];
    EdgeDistance = ShapeConstants.LineWidth;
    gl_Position = vec4((p0 + line0) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw);
    EmitVertex();

    Color = colors[1];
    EdgeDistance = -ShapeConstants.LineWidth;
    gl_Position = vec4((p1 - line0) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw);
    EmitVertex();

    Color = colors[1];
    EdgeDistance = ShapeConstants.LineWidth;
    gl_Position = vec4((p1 + line0) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw);
    EmitVertex();
    EndPrimitive();
}

//------------------------------------------------------------------------------
/**
*/
[input_primitive] = triangles
[output_primitive] = triangle_strip
[max_vertex_count] = 3
shader
void
gsTriangles(
    in vec4 colors[],
    out vec4 Color,
    out vec3 Distance
)
{
    Color = colors[0];
    Distance = vec3(1, 0, 0);
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    Color = colors[1];
    Distance = vec3(0, 1, 0);
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    Color = colors[2];
    Distance = vec3(0, 0, 1);
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    EndPrimitive();
    
}

//------------------------------------------------------------------------------
/**
*/
float 
evalMinDistanceToEdges(vec3 heights)
{
    float3 ddxHeights = dFdx(heights);
    float3 ddyHeights = dFdy(heights);
    float3 ddHeights2 = ddxHeights * ddxHeights + ddyHeights * ddyHeights;

    float3 pixHeights2 = heights * heights / ddHeights2;

    float dist = sqrt(min(min(pixHeights2.x, pixHeights2.y), pixHeights2.z));

    return dist;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psTriangles(
    in vec4 color,
    in vec3 distance,
    [color0] out vec4 Color)
{
    float dist = evalMinDistanceToEdges(distance);
    float halfLineWidth = 0.5f * ShapeConstants.LineWidth;
    if (dist > halfLineWidth + 1.0f)
        discard;

    dist = clamp((dist - (halfLineWidth - 1.0f)), 0, 2.0f);
    dist *= dist;
    float alpha = exp2(-2 * dist);
    Color = vec4(color.rgb, color.a * alpha);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psLines(
    in noperspective float edgeDistance,
    in vec4 color,
    [color0] out vec4 Color)
{
    float d = abs(edgeDistance) / ShapeConstants.LineWidth;
    d = smoothstep(1.0, 1.0 - (2.0f / ShapeConstants.LineWidth), d);
    Color = vec4(color.rgb, color.a * d);
}


//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vec4 color,
    [color0] out vec4 Color) 
{
    Color = color;
}

GeometryTechnique(MeshWireframeTriangles, "Mesh|Wireframe|Triangles", vsMesh(), gsTriangles(), psTriangles(), DepthEnabledState);
GeometryTechnique(MeshWireframeLines, "Mesh|Wireframe|Lines", vsMesh(), gsLines(), psLines(), DepthEnabledState);
GeometryTechnique(PrimtivesWireframeTriangles, "Primitives|Wireframe|Triangles", vsPrimitives(), gsTriangles(), psTriangles(), DepthEnabledState);
GeometryTechnique(PrimitivesWireframeLines, "Primitives|Wireframe|Lines", vsPrimitives(), gsLines(), psLines(), DepthEnabledState);

SimpleTechnique(Mesh, "Mesh", vsMesh(), psMain(), DepthEnabledState);
SimpleTechnique(Primitives, "Primitives", vsPrimitives(), psMain(), DepthEnabledState);
SimpleTechnique(MeshNoDepth, "Mesh|NoDepth", vsMesh(), psMain(), DepthDisabledState);
SimpleTechnique(PrimitivesNoDepth, "Primitives|NoDepth", vsPrimitives(), psMain(), DepthDisabledState);