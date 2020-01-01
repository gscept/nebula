#pragma once
#ifndef HTTP_HTMLELEMENT_H
#define HTTP_HTMLELEMENT_H
//------------------------------------------------------------------------------
/**
    @class Http::HtmlElement
    
    HTML markup elements.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Http
{
class HtmlElement
{
public:
    /// elements
    enum Code
    {
        Html,                               //> <html>
        Head,                               //> <head>
        Title,                              //> <title>
        Body,                               //> <body>
        Heading1,                           //> <h1>
        Heading2,                           //> <h2>
        Heading3,                           //> <h3>
        Heading4,                           //> <h4>
        Heading5,                           //> <h5>
        Heading6,                           //> <h6>
        Paragraph,                          //> <p>
        UnorderedList,                      //> <ul>
        OrderedList,                        //> <ol>
        ListItem,                           //> <li>
        DefinitionList,                     //> <dl>
        DefinitionListTerm,                 //> <dt>
        DefinitionListDefinition,           //> <dd>
        BlockQuote,                         //> <blockquote>
        PreFormatted,                       //> <pre>
        Bold,                               //> <b>
        Italics,                            //> <i>
        Teletyper,                          //> <tt>
        Underscore,                         //> <u>
        Strike,                             //> <strike>
        Big,                                //> <big>
        Small,                              //> <small>
        Sup,                                //> <sup>
        Sub,                                //> <sub>
        Table,                              //> <table>
        TableRow,                           //> <tr>
        TableHeader,                        //> <th>
        TableData,                          //> <td>
        Anchor,                             //> <a>
        Image,                              //> <img>
        Object,                             //> <object>
        Form,                               //> <form>
        Input,                              //> <input>
        Div,                                //> <div>
        Span,                               //> <span>
        Style,                              //> <style>
        Font,                               //> <font>
        Script,                             //> <script>

        InvalidHtmlElement,
    };     
    
    /// convert to string
    static Util::String ToHtml(Code c);
};

} // namespace Http
//------------------------------------------------------------------------------
#endif
    