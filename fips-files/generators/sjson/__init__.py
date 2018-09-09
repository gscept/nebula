"""Module to parse SJSON files."""
# coding=utf8
# @author: Matthaus G. Chajdas
# @license: 3-clause BSD

import collections
import numbers
import string
import io

__version__ = 'N.2.0.3'


class MemoryInputStream:
    """Input stream wrapper for reading directly from memory."""
    def __init__(self, s):
        """
        s -- a bytes object.
        """
        self._stream = s
        self._current_index = 0
        self._length = len(s)

    def read(self, count=1):
        """read ``count`` bytes from the stream."""
        end_index = self._current_index + count
        if end_index > self._length:
            _raise_end_of_file_exception(self)
        result = self._stream[self._current_index:end_index]
        self._current_index = end_index
        return result

    def peek(self, count=1, allow_end_of_file=False):
        """peek ``count`` bytes from the stream. If ``allow_end_of_file`` is
        ``True``, no error will be raised if the end of the stream is reached
        while trying to peek."""
        end_index = self._current_index + count
        if end_index > self._length:
            if allow_end_of_file:
                return None
            _raise_end_of_file_exception(self)

        return self._stream[self._current_index:end_index]

    def skip(self, count=1):
        """skip ``count`` bytes."""
        self._current_index += count

    def get_location(self):
        """Get the current location in the stream."""
        loc = collections.namedtuple('Location', ['line', 'column'])
        bytes_read = self._stream[:self._current_index]
        line = 1
        column = 1
        for byte in bytes_read:
            # We test the individual bytes here, must use ord
            if byte == ord('\n'):
                line += 1
                column = 1
            else:
                column += 1
        return loc(line, column)


class ByteBufferInputStream:
    """Input stream wrapper for reading directly from an I/O object."""
    def __init__(self, stream):
        self._stream = stream
        self._index = 0
        self._line = 1
        self._column = 1

    def read(self, count=1):
        """read ``count`` bytes from the stream."""
        result = self._stream.read(count)
        if len(result) < count:
            _raise_end_of_file_exception(self)

        for char in result:
            # We test the individual bytes here, must use ord
            if char == ord('\n'):
                self._line += 1
                self._column = 1
            else:
                self._column += 1
        return result

    def peek(self, count=1, allow_end_of_file=False):
        """peek ``count`` bytes from the stream. If ``allow_end_of_file`` is
        ``True``, no error will be raised if the end of the stream is reached
        while trying to peek."""
        result = self._stream.peek(count)
        if not result and not allow_end_of_file:
            _raise_end_of_file_exception(self)
        elif not result and allow_end_of_file:
            return None
        else:
            return result[:count]

    def skip(self, count=1):
        """skip ``count`` bytes."""
        self.read(count)

    def get_location(self):
        """Get the current location in the stream."""
        loc = collections.namedtuple('Location', ['line', 'column'])
        return loc(self._line, self._column)


class ParseException(RuntimeError):
    """Parse exception."""
    def __init__(self, msg, location):
        super(ParseException, self).__init__(msg)
        self._msg = msg
        self._location = location

    def get_location(self):
        """Get the current location at which the exception occurred."""
        return self._location

    def __str__(self):
        return '{} at line {}, column {}'.format(self._msg,
                                                 self._location.line,
                                                 self._location.column)


def _raise_end_of_file_exception(stream):
    raise ParseException('Unexpected end-of-stream', stream.get_location())


def _consume(stream, what):
    _skip_whitespace(stream)
    what_len = len(what)
    if stream.peek(what_len) != what:
        raise ParseException("Expected to read '{}'".format(what),
                             stream.get_location())
    stream.skip(what_len)


def _skip_characterse_and_whitespace(stream, num_char_to_skip):
    stream.skip(num_char_to_skip)
    return _skip_whitespace(stream)

_WHITESPACE_SET = set({b' ', b'\t', b'\n', b'\r'})


def _is_whitespace(char):
    return char in _WHITESPACE_SET


def _skip_c_style_comment(stream):
    comment_start_location = stream.get_location()
    # skip the comment start
    stream.skip(2)
    # we don't support nested comments, so we're not going to
    # count the nesting level. Instead, skip ahead until we
    # find a closing */
    while True:
        next_char = stream.peek(1, allow_end_of_file=True)
        if next_char == b'*':
            comment_end = stream.peek(2, allow_end_of_file=True)
            if comment_end == b'*/':
                stream.skip(2)
                break
            else:
                stream.skip()
        elif next_char is None:
            raise ParseException("Could not find closing '*/' for comment",
                                 comment_start_location)
        stream.skip()


def _skip_cpp_style_comment(stream):
    # skip the comment start
    stream.skip(2)
    while True:
        next_char = stream.peek(allow_end_of_file=True)
        if next_char is None or next_char == b'\n':
            break
        stream.skip()


def _skip_whitespace(stream):
    '''skip whitespace. Returns the next character if a new position within the
    stream was found; returns None if the end of the stream was hit.'''
    while True:
        next_char = stream.peek(allow_end_of_file=True)
        if not _is_whitespace(next_char):
            if next_char == b'/':
                # this could be a C or C++ style comment
                comment_start = stream.peek(2, allow_end_of_file=True)
                if comment_start == b'/*':
                    _skip_c_style_comment(stream)
                    continue
                elif comment_start == b'//':
                    _skip_cpp_style_comment(stream)
                    continue
            break
        stream.skip()

    return next_char

_IDENTIFIER_SET = set(string.ascii_letters + string.digits + '_')


def _is_identifier(obj):
    return chr(obj[0]) in _IDENTIFIER_SET


def _decode_escaped_character(char):
    if char == b'b':
        return b'\b'
    elif char == b'n':
        return b'\n'
    elif char == b't':
        return b'\t'
    elif char == b'\\' or char == b'\"':
        return char


def _decode_string(stream, allow_identifier=False):
    _skip_whitespace(stream)

    result = bytearray()

    is_quoted = stream.peek() == b'\"' or stream.peek() == b'['
    if not allow_identifier and not is_quoted:
        raise ParseException('Quoted string expected', stream.get_location())

    raw_quotes = False
    if is_quoted and stream.peek() == b'[':
        if stream.read(3) == b'[=[':
            raw_quotes = True
        else:
            raise ParseException('Raw quoted string must start with [=[',
                                 stream.get_location())
    elif is_quoted and stream.peek() == b'\"':
        stream.skip()

    parse_as_identifier = False
    if not is_quoted:
        parse_as_identifier = True

    while True:
        next_char = stream.peek()
        if parse_as_identifier and not _is_identifier(next_char):
            break

        if raw_quotes:
            if next_char == b']' and stream.peek(3) == b']=]':
                stream.skip(3)
                break
            else:
                result += next_char
                stream.skip(1)
        else:
            if next_char == b'\"':
                stream.read()
                break
            elif next_char == b'\\':
                stream.skip()
                result += _decode_escaped_character(stream.read())
            else:
                result += next_char
                stream.skip()

    return str(result)

_NUMBER_SEPARATOR_SET = _WHITESPACE_SET.union(set({b',', b']', b'}', None}))


def _decode_number(stream, next_char):
    """Parse a number.

    next_char -- the next byte in the stream.
    """
    number_bytes = bytearray()
    is_decimal_number = False

    while True:
        if next_char in _NUMBER_SEPARATOR_SET:
            break

        if next_char == b'.' or next_char == b'e' or next_char == b'E':
            is_decimal_number = True

        number_bytes += next_char
        stream.skip()

        next_char = stream.peek(allow_end_of_file=True)

    value = number_bytes.decode('utf-8')

    if is_decimal_number:
        return float(value)
    return int(value)


def _decode_dict(stream, delimited=False):
    """
    delimited -- if ``True``, parsing will stop once the end-of-dictionary
                 delimiter has been reached(``}``)
    """
    from collections import OrderedDict
    result = OrderedDict()

    if stream.peek() == b'{':
        stream.skip()

    next_char = _skip_whitespace(stream)

    while True:
        if not delimited and next_char is None:
            break

        if next_char == b'}':
            stream.skip()
            break

        key = _decode_string(stream, True)
        next_char = _skip_whitespace(stream)
        # We allow both '=' and ':' as separators inside maps
        if next_char == b'=' or next_char == b':':
            _consume(stream, next_char)
        value = _parse(stream)
        result[key] = value

        next_char = _skip_whitespace(stream)
        if next_char == b',':
            next_char = _skip_characterse_and_whitespace(stream, 1)

    return result


def _parse_list(stream):
    result = []
    # skip '['
    next_char = _skip_characterse_and_whitespace(stream, 1)

    while True:
        if next_char == b']':
            stream.skip()
            break

        value = _parse(stream)
        result.append(value)

        next_char = _skip_whitespace(stream)
        if next_char == b',':
            next_char = _skip_characterse_and_whitespace(stream, 1)

    return result


def _parse(stream):
    next_char = _skip_whitespace(stream)

    if next_char == b't':
        _consume(stream, b'true')
        return True
    elif next_char == b'f':
        _consume(stream, b'false')
        return False
    elif next_char == b'n':
        _consume(stream, b'null')
        return None
    elif next_char == b'{':
        return _decode_dict(stream, True)
    elif next_char == b'\"':
        return _decode_string(stream)
    elif next_char == b'[':
        peek = stream.peek(2, allow_end_of_file=False)
        # second lookup character for [=[]=] raw literal strings
        next_char_2 = peek[1:2]
        if next_char_2 != b'=':
            return _parse_list(stream)
        elif next_char_2 == b'=':
            return _decode_string(stream)

    try:
        return _decode_number(stream, next_char)
    except ValueError:
        raise ParseException('Invalid character', stream.get_location())


def load(stream):
    """Load a SJSON object from a stream."""
    return _decode_dict(ByteBufferInputStream(io.BufferedReader(stream)))


def loads(text):
    """Load a SJSON object from a string."""
    return _decode_dict(MemoryInputStream(text.encode('utf-8')))


def dumps(obj, indent=None):
    """Dump an object to a string."""
    import io
    stream = io.StringIO()
    dump(obj, stream, indent)
    return stream.getvalue()


def dump(obj, fp, indent=None):
    """Dump an object to a stream."""
    if not indent:
        _indent = ''
    elif isinstance(indent, numbers.Number):
        if indent < 0:
            indent = 0
        _indent = ' ' * indent
    else:
        _indent = indent

    for e in _encode(obj, indent=_indent):
        fp.write(e)

_ESCAPE_CHARACTER_SET = {'\n': '\\n', '\b': '\\b', '\t': '\\t', '\"': '\\"'}


def _escape_string(obj, quote=True):
    """Escape a string.

    If quote is set, the string will be returned with quotation marks at the
    beginning and end. If quote is set to false, quotation marks will be only
    added if needed(that is, if the string is not an identifier.)"""
    if any([c not in _IDENTIFIER_SET for c in obj]):
        # String must be quoted, even if quote was not requested
        quote = True

    if quote:
        yield '"'

    for key, value in _ESCAPE_CHARACTER_SET.items():
        obj = obj.replace(key, value)

    yield obj

    if quote:
        yield '"'


def _encode(obj, separators=(', ', '\n', ' = '), indent=0, level=0):
    if obj is None:
        yield 'null'
    # Must check for true, false before number, as boolean is an instance of
    # Number, and str(obj) would return True/False instead of true/false then
    elif obj is True:
        yield 'true'
    elif obj is False:
        yield 'false'
    elif isinstance(obj, numbers.Number):
        yield str(obj)
    # Strings are also Sequences, but we don't want to encode as lists
    elif isinstance(obj, str):
        for temp in _escape_string(obj):
            yield temp
    elif isinstance(obj, collections.abc.Sequence):
        for temp in _encode_list(obj, separators, indent, level):
            yield temp
    elif isinstance(obj, collections.abc.Mapping):
        for temp in _encode_dict(obj, separators, indent, level):
            yield temp
    else:
        raise RuntimeError("Unsupported object type")


def _indent(level, indent):
    return indent * level


def _encode_key(k):
    for temp in _escape_string(k, False):
        yield temp

def _encode_list(obj, separators, indent, level):
    yield '['
    first = True
    for element in obj:
        if first:
            first = False
        else:
            yield separators[0]
        for temp in _encode(element, separators, indent, level+1):
            yield temp

    yield ']'


def _encode_dict(obj, separators, indent, level):
    if level > 0:
        yield '{\n'
    first = True
    for key, value in obj.items():
        if first:
            first = False
        else:
            yield '\n'
        yield _indent(level, indent)
        for temp in _encode_key(key):
            yield temp
        yield separators[2]
        for temp in _encode(value, separators, indent, level+1):
            yield temp
    yield '\n'
    yield _indent(level-1, indent)
    if level > 0:
        yield '}'
