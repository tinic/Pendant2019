/*
Copyright 2019 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
int __attribute__((weak)) _read (int file, char * ptr, int len)
{
    int nChars = 0;

    if (file != 0) {
        return -1;
    }

    for (; len > 0; --len) {
        //      ptr_get(stdio_base, ptr);
        ptr++;
        nChars++;
    }
    return nChars;
}

int __attribute__((weak)) _write (int file, const char *ptr, int len)
{
    (void) ptr;
    int nChars = 0;

    if ((file != 1) && (file != 2) && (file!=3)) {
        return -1;
    }

    for (; len != 0; --len) {
        //      if (ptr_put(stdio_base, *ptr++) < 0) {
        //          return -1;
        //      }
        ++nChars;
    }
    return nChars;
}
