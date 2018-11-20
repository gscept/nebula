//------------------------------------------------------------------------------
//  im3d.fx
//
//	Shader for Im3d rendering
//
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/techniques.fxh" 
#include "lib/shared.fxh"

state Im3dState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    DepthWrite = false;
    DepthEnabled = false;
    CullMode = None;
    ScissorEnabled = false;
};
state Im3dDepthState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    DepthWrite = false;
    DepthEnabled = true;
    CullMode = None;
    ScissorEnabled = false;
};

#define kAntialiasing 2.0

shader
void
vsMainLines(
    [slot = 0] in vec4 position_size,
    [slot = 1] in vec4 color,    
    out noperspective float size,
    out vec4 Color
)
{
	Color = color.abgr;
    Color.a *= smoothstep(0.0, 1.0, position_size.w / kAntialiasing);    
	size = max(position_size.w, kAntialiasing);	
	gl_Position = ViewProjection * vec4(position_size.xyz, 1.0);	
}

shader
void
vsMainPoints(
    [slot = 0] in vec4 position_size,
    [slot = 1] in vec4 color,    
    out noperspective float size,
    out vec4 Color
)
{    
    Color = color.abgr;
    Color.a *= smoothstep(0.0, 1.0, position_size.w / kAntialiasing);    
    size = max(position_size.w, kAntialiasing);

    gl_Position = ViewProjection * vec4(position_size.xyz, 1.0);    
    gl_PointSize = size;    
}

shader
void
vsMainTriangles(
    [slot = 0] in vec4 position_size,
    [slot = 1] in vec4 color,    
    out noperspective float size,
    out vec4 Color
)
{
    Color = color.abgr;
    size = max(position_size.w, kAntialiasing);
    gl_Position = ViewProjection * vec4(position_size.xyz, 1.0);    
}

[inputprimitive] = lines
[outputprimitive] = triangle_strip
[maxvertexcount] = 4
shader
void
gsMain(     
    in noperspective float size[],
    in vec4 color[],		
    out noperspective float EdgeDistance,
    out noperspective float Size,
    out vec4 Color
)
{
	vec2 pos0 = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
	vec2 pos1 = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;
		
	vec2 dir = pos0 - pos1;
	dir = normalize(vec2(dir.x, dir.y * RenderTargetDimensions[0].y * RenderTargetDimensions[0].z)); // correct for aspect ratio
	vec2 tng0 = vec2(-dir.y, dir.x);
    vec2 tng1 = tng0 * size[1] * RenderTargetDimensions[0].zw;
    tng0 = tng0 * size[0] * RenderTargetDimensions[0].zw;
		
	// line start
	gl_Position = vec4((pos0 - tng0) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw); 
	EdgeDistance = -size[0];
	Size = size[0];
	Color = color[0];
	EmitVertex();
		
	gl_Position = vec4((pos0 + tng0) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw);
	Color = color[0];
	EdgeDistance = size[0];
	Size = size[0];
	EmitVertex();
		
	// line end
	gl_Position = vec4((pos1 - tng1) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw);
	EdgeDistance = -size[1];
	Size = size[1];
	Color = color[1];
	EmitVertex();
		
	gl_Position = vec4((pos1 + tng1) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw);
	Color = color[1];
	Size = size[1];
	EdgeDistance = size[1];
	EmitVertex();
    EndPrimitive();
}


shader
void
psMainTriangles(    
    in noperspective float size,
    in vec4 color,	
	[color0] out vec4 finalColor
)
{
    finalColor = color;        
}

shader
void
psMainLines(
    in noperspective float edgeDistance,
    in noperspective float size,
    in vec4 color,
    [color0] out vec4 finalColor
)
{
    finalColor = color;
    float d = abs(edgeDistance) / size;
    d = smoothstep(1.0, 1.0 - (kAntialiasing / size), d);
    finalColor.a *= d;
}

shader
void
psMainPoints(    
    in noperspective float size,
    in vec4 color,
    [color0] out vec4 finalColor
)
{
    finalColor = color;
    float d = length(gl_PointCoord.xy - vec2(0.5));
    d = smoothstep(0.5, 0.5 - (kAntialiasing / size), d);    
    finalColor.a *= d;
}

GeometryTechnique(Lines, "Static|Lines", vsMainLines(), psMainLines(), gsMain(), Im3dState);
GeometryTechnique(LinesDepth, "StaticDepth|Lines", vsMainLines(), psMainLines(), gsMain(), Im3dDepthState);
SimpleTechnique(Points, "Static|Points", vsMainPoints(), psMainPoints(), Im3dState);
SimpleTechnique(Triangles, "Static|Triangles", vsMainTriangles(), psMainTriangles(), Im3dState);
SimpleTechnique(TrianglesDepth, "StaticDepth|Triangles", vsMainTriangles(), psMainTriangles(), Im3dDepthState);
