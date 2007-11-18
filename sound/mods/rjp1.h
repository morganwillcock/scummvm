/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#ifndef SOUND_MODS_RJP1_H
#define SOUND_MODS_RJP1_H

#include "common/stream.h"

namespace Audio {

class AudioStream;

/*
 * FIXME: Document this function. In particular, who is responsible for
 * freeing the data passed in, resp. the AudioStream it returns?
 */
AudioStream *makeRjp1Stream(Common::SeekableReadStream *songData, Common::SeekableReadStream *instrumentsData, int num, int rate = 44100, bool stereo = true);

} // End of namespace Audio

#endif
