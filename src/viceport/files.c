/*
 *  THEC64 Mini
 *  Copyright (C) 2017 Chris Smith
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define _XOPEN_SOURCE 500 // Mandatory for nftw()
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ftw.h>
#include <unistd.h>

// ----------------------------------------------------------------------------------
//
static int
_nftw_cb( const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = 0;
    if(  strncmp( fpath, "/tmp/vice.", 10 ) != 0 ) return 0;

    rv = remove(fpath);
    if (rv) perror(fpath);

    return rv;
}

// ----------------------------------------------------------------------------------
//
int
delete_temporary_files()
{
    return nftw("/tmp", _nftw_cb, 64, FTW_DEPTH | FTW_PHYS | FTW_SKIP_SUBTREE );
}

// ----------------------------------------------------------------------------------
// end
