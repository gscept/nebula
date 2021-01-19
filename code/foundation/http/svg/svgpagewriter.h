#pragma once
#ifndef HTTP_SVGPAGEWRITER_H
#define HTTP_SVGPAGEWRITER_H
//------------------------------------------------------------------------------
/**
    @class Http::SvgPageWriter
    
    A stream writer to generate simple SVG pages.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/streamwriter.h"
#include "io/xmlwriter.h"
#include "math/vec2.h"

//------------------------------------------------------------------------------
namespace Http
{
class SvgPageWriter : public IO::StreamWriter
{
    __DeclareClass(SvgPageWriter);
public:
    /// constructor
    SvgPageWriter();
    /// destructor
    virtual ~SvgPageWriter();
    /// set width and height of canvas in pixels
    void SetCanvasDimensions(SizeT w, SizeT h);
    /// set width and height in draw units
    void SetUnitDimensions(SizeT w, SizeT h);
    /// begin writing to the stream
    virtual bool Open();
    /// end writing to the stream
    virtual void Close();

    /// write content text
    void WriteContent(const Util::String& text);
    /// begin a new node under the current node
    void BeginNode(const Util::String& nodeName);
    /// end current node, set current node to parent
    void EndNode();
    /// set string attribute on current node
    void SetString(const Util::String& name, const Util::String& value);

    /// begin a transformation group
    void BeginTransformGroup(const Math::vec2& translate, float rotate, const Math::vec2& scale);
    /// begin a paint group
    void BeginPaintGroup(const Util::String& fillColor, const Util::String& strokeColor, int strokeWidth);
    /// begin a text group
    void BeginTextGroup(int fontSize, const Util::String& textColor);
    /// end the most recent group
    void EndGroup();

    /// draw a rectangle, all units are in pixels
    void Rect(float x, float y, float w, float h);
    /// draw a circle, all units are in pixels
    void Circle(float x, float y, float r);
    /// draw an ellipse, all units are in pixels
    void Ellipse(float x, float y, float rx, float ry);
    /// draw a line, all units are in pixels
    void Line(float x1, float y1, float x2, float y2);
    /// draw a poly-line, all units are in pixels
    void PolyLine(const Util::Array<Math::vec2>& points);
    /// draw a polygon, all units are in pixels
    void Polygon(const Util::Array<Math::vec2>& points);
    /// draw text, all units are in pixels
    void Text(float x, float y, const Util::String& text);

protected:
    Ptr<IO::XmlWriter> xmlWriter;
    SizeT canvasWidth;
    SizeT canvasHeight;
    SizeT unitWidth;
    SizeT unitHeight;
};

//------------------------------------------------------------------------------
/**
*/
inline void
SvgPageWriter::SetCanvasDimensions(SizeT w, SizeT h)
{
    this->canvasWidth = w;
    this->canvasHeight = h;
}

//------------------------------------------------------------------------------
/**
*/
inline void
SvgPageWriter::SetUnitDimensions(SizeT w, SizeT h)
{
    this->unitWidth = w;
    this->unitHeight = h;
}

} // namespace Http
//------------------------------------------------------------------------------
#endif
    