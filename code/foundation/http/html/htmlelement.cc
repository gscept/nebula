//------------------------------------------------------------------------------
//  htmlelement.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "http/html/htmlelement.h"

namespace Http
{

//------------------------------------------------------------------------------
/**
*/
Util::String
HtmlElement::ToHtml(Code c)
{
    switch (c)
    {
        case Html:                               return "html";
        case Head:                               return "head";
        case Title:                              return "title";
        case Body:                               return "body";
        case Heading1:                           return "h1";
        case Heading2:                           return "h2";
        case Heading3:                           return "h3";
        case Heading4:                           return "h4";
        case Heading5:                           return "h5";
        case Heading6:                           return "h6";
        case Paragraph:                          return "p";
        case UnorderedList:                      return "ul";
        case OrderedList:                        return "ol";
        case ListItem:                           return "li";
        case DefinitionList:                     return "dl";
        case DefinitionListTerm:                 return "dt";
        case DefinitionListDefinition:           return "dd";
        case BlockQuote:                         return "blockquote";
        case PreFormatted:                       return "pre";
        case Bold:                               return "b";
        case Italics:                            return "i";
        case Teletyper:                          return "tt";
        case Underscore:                         return "u";
        case Strike:                             return "strike";
        case Big:                                return "big";
        case Small:                              return "small";
        case Sup:                                return "sup";
        case Sub:                                return "sub";
        case Table:                              return "table";
        case TableRow:                           return "tr";
        case TableHeader:                        return "th";
        case TableData:                          return "td";
        case Anchor:                             return "a";
        case Image:                              return "img";
        case Object:                             return "object"; 
        case Form:                               return "form";
        case Input:                              return "input";
        case Div:                                return "div";
        case Span:                               return "span";
        case Style:                              return "style";
        case Font:                               return "font";
        case Script:                             return "script";
        default:
            n_error("HtmlElement::ToHtml(): invalid element code!");
            return "";
    }
}

} // namespace Http