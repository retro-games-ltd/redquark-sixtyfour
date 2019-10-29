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
#ifndef SEM_H
#define SEM_H

typedef enum {
    SEM_FRAME_DONE = 0,
    SEM_START_NEXT_FRAME,
    SEM_SOUND_FLUSH,
    SEM_SOUND_FLUSH_DONE,
    SEM_PAUSE_RELEASE,
    SEM_SAVE_LOAD_DONE,
    SEM_PAUSED,
    SEM_MAX
} semaphore_t;

int semaphore_init();
void semaphore_wait_for( semaphore_t s );
void semaphore_notify( semaphore_t s );

#endif
