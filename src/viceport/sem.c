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
#include <semaphore.h>
#include <errno.h>

#include "sem.h"

static sem_t semaphores[SEM_MAX];

// ----------------------------------------------------------------------------
int
semaphore_init()
{
    int i;
    for(i = 0; i < SEM_MAX; i++) {
        if( sem_init( semaphores + i, 0, 0 ) < 0 ) {
            return -1;
        }
    }
}

// ----------------------------------------------------------------------------
void 
semaphore_wait_for( semaphore_t s )
{
    if( s >= SEM_MAX ) return;

#ifdef DEBUG
    int c = 0;
    if( sem_wait( semaphores + s ) == 0 ) {
        // Are there others to comsume?
        do {
            c++;
        } while ( sem_trywait( semaphores + s ) == 0 );
        if(c > 1) printf("sem_sait consumed %d semphores %d\n", c, s );
    }
#else
    if( sem_wait( semaphores + s ) == 0 ) {
        while ( sem_trywait( semaphores + s ) == 0 ) { } // Eat all pedning, in case there are multiple
    }
#endif
}

// ----------------------------------------------------------------------------
void 
semaphore_notify( semaphore_t s )
{
    if( s >= SEM_MAX ) return;

    //if( sem_trywait( semaphores + s ) == 0 ) {
    //    printf("Semaphore %d already posted\n", s );
    //}

    sem_post( semaphores + s );
}

// ----------------------------------------------------------------------------
// end
