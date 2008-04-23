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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#ifndef KYRA_SCREEN_V3_H
#define KYRA_SCREEN_V3_H

#include "kyra/screen.h"

namespace Kyra {

class KyraEngine_v3;

class Screen_v3 : public ScreenEx {
public:
	Screen_v3(KyraEngine_v3 *vm, OSystem *system);
	virtual ~Screen_v3();

	virtual void setScreenDim(int dim);
	virtual const ScreenDim *getScreenDim(int dim);

	int getLayer(int x, int y);

	byte getShapeFlag1(int x, int y);
	byte getShapeFlag2(int x, int y);

	int getDrawLayer(int x, int y);
	int getDrawLayer2(int x, int y, int height);
private:
	static const ScreenDim _screenDimTable[];
	static const int _screenDimTableCount;
};

} // end of namespace Kyra

#endif
