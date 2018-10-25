#pragma once
//------------------------------------------------------------------------------
/**
	@class CoreGraphics::ImageFileFormat
	
	Image file formats supported by StreamTextureSaver.
	
	(C) 2007 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "util/string.h"
#include "io/mediatype.h"

namespace CoreGraphics
{
class ImageFileFormat
{
public:
	/// image file formats
	enum Code
	{
		BMP,
		JPG,
		PNG,
		DDS,
		TGA,

		InvalidImageFileFormat,
	};
	
	/// convert from string
	static Code FromString(const Util::String& str);
	/// convert to string
	static Util::String ToString(Code c);
	/// convert from media type (MIME)
	static Code FromMediaType(const IO::MediaType& mediaType);
	/// convert to media type (MIME)
	static IO::MediaType ToMediaType(Code c);
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------

	