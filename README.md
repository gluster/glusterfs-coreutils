# gluster-coreutils

The GlusterFS Coreutils is a suite of utilities that aims to mimic the standard
Linux coreutils, with the exception that it utilizes the gluster C API in order
to do work.

## Building

Building requires a few dependencies:

1. `glusterfs-api-devel`
1. `libtirpc-devel`
1. `help2man`
1. `autoconf >= 2.69`
1. `automake >= 1.15`

### Git

`$ ./autogen.sh && ./configure && make`

### Release

`$ ./configure && make`

### RPM

`$ make rpms`

## License

Copyright (C) 2015 Facebook, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

See `COPYING` for full license.
