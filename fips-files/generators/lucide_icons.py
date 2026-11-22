"""
Generate the icon table the editor draws.

The table is built from the font rather than from the codepoint map Lucide publishes, because the two
do not agree. The published map has entries for icons the published font does not have, and a few
hundred of them resolve to a different glyph than the name says. The font is what ImGui rasterizes, so
the font is the source of truth.

The input is the name list in the module. The font comes as an argument, because it lives in the
engine's font folder. The output is the table and the codepoint ranges ImGui bakes.
"""
Version = 2

import pathlib

from fontTools.ttLib import TTFont

import genutil as util

BANNER = '//------------------------------------------------------------------------------'
TAG = '// Icon Registry #version:{}#'.format(Version)


def read_names(path) :
    """read the icon names, one per line. Empty lines and comments are skipped"""
    names = []
    for line in path.read_text().splitlines() :
        line = line.strip()
        if not line or line.startswith('#') :
            continue
        names.append(line)
    return names


def utf8_escape(codepoint) :
    """the codepoint as the UTF-8 bytes a string literal needs"""
    return ''.join('\\x%02X' % byte for byte in chr(codepoint).encode('utf-8'))


def codepoints(names, font_path) :
    """the name to codepoint map. The cmap is codepoint to glyph name, so this is its inverse"""
    font = TTFont(font_path)
    by_name = {}
    for codepoint, glyph in font.getBestCmap().items() :
        by_name[glyph] = codepoint

    missing = [name for name in names if name not in by_name]
    if missing :
        util.fmtError('Names the font does not have: ' + ', '.join(missing))
    return sorted((name, by_name[name]) for name in names)


def glyph_ranges(entries) :
    """the codepoints as the range list ImGui reads. Two values per range, values inclusive, the list
    ends with zero. Consecutive codepoints become one range"""
    cps = sorted(set(cp for _, cp in entries))
    ranges = []
    i = 0
    while i < len(cps) :
        start = cps[i]
        end = start
        while i + 1 < len(cps) and cps[i + 1] == end + 1 :
            end = cps[i + 1]
            i += 1
        ranges.append((start, end))
        i += 1
    return ranges


def generateHeader(hdrPath, entries) :
    with open(hdrPath, 'w') as f :
        f.write(TAG + '\n')
        for line in [
            BANNER,
            '/**',
            '    The icon glyphs the editor draws.',
            '',
            '    This file was generated with Nebula\'s IDL compiler tool.'
            '    DO NOT EDIT',
            '',
            '    @copyright',
            '    (C) 2026 Individual contributors, see AUTHORS file',
            '*/',
            BANNER,
            '#pragma once',
            '#include "foundation/stdneb.h"',
            '#include "imgui.h"',
            '',
            'namespace Editor',
            '{',
            '',
            'struct IconEntry',
            '{',
            '    const char* name;',
            '    /// the codepoint as UTF-8, so a glyph is drawn as an ordinary string',
            '    const char utf8[4];',
            '};',
            '',
            '/// the table, sorted by name so the lookup is a binary search',
            'extern const IconEntry Icons[];',
            'extern const IndexT NumIcons;',
            '',
            '/// the glyph for an icon name, or nullptr when the name is not in the table',
            'const char* IconGlyph(const char* name);',
            '',
            '/// the codepoints as the range list ImGui reads. Two values per range, values inclusive,',
            '/// the list ends with zero. The codepoints are in the private use area, which is outside',
            '/// the range ImGui bakes by default',
            'extern const ImWchar IconGlyphRanges[];',
            '',
            '} // namespace Editor',
            ''
        ] :
            f.write(line + '\n')


def generateSource(srcPath, entries) :
    with open(srcPath, 'w') as f :
        f.write(TAG + '\n')
        for line in ([
            BANNER,
            '//  @file iconregistry.cc',
            '//  This file was generated with Nebula\'s IDL compiler tool.'
            '//  DO NOT EDIT',
            '//  @copyright (C) 2026 Individual contributors, see AUTHORS file',
            BANNER,
            '#include "foundation/stdneb.h"',
            '#include "neweditor/base/iconregistry.h"',
            '',
            'namespace Editor',
            '{',
            '',
            'const IconEntry Icons[] = {',
        ] + ['    { "%s", "%s" },   // U+%04X' % (name, utf8_escape(codepoint), codepoint)
             for name, codepoint in entries] + [
            '};',
            '',
            'const IndexT NumIcons = %d;' % len(entries),
            '',
            'const ImWchar IconGlyphRanges[] = {',
        ] + ['    0x%04X, 0x%04X,' % (start, end) for start, end in glyph_ranges(entries)] + [
            '    0,',
            '};',
            '',
            BANNER,
            '/**',
            '    The table is sorted, so this is a binary search. The comparison is written out rather than using',
            '    a CRT string function, because the names are short.',
            '*/',
            'const char*',
            'IconGlyph(const char* name)',
            '{',
            '    IndexT lo = 0;',
            '    IndexT hi = NumIcons - 1;',
            '    while (lo <= hi)',
            '    {',
            '        const IndexT mid = (lo + hi) >> 1;',
            '',
            '        const char* a = name;',
            '        const char* b = Icons[mid].name;',
            '        while (*a != 0 && *b != 0 && *a == *b)',
            '        {',
            '            ++a;',
            '            ++b;',
            '        }',
            '',
            '        if (*a == 0 && *b == 0)',
            '            return Icons[mid].utf8;',
            '',
            '        if (*a == 0 || *a < *b)',
            '            hi = mid - 1;',
            '        else',
            '            lo = mid + 1;',
            '    }',
            '    return nullptr;',
            '}',
            '',
            '} // namespace Editor',
            ''
        ]) :
            f.write(line + '\n')


def generate(input, out_src, out_hdr, args) :
    if util.isDirty(Version, [input], [out_src, out_hdr]) :
        font = args['font']
        if not font or not pathlib.Path(font).is_file() :
            util.fmtError('Icon font not found: {}'.format(font))
        entries = codepoints(read_names(pathlib.Path(input)), font)
        generateHeader(out_hdr, entries)
        generateSource(out_src, entries)
