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
 */

/* Based on the Adventure Game Studio source code, copyright 1999-2011 Chris Jones,
 * which is licensed under the Artistic License 2.0.
 */

// Base stuff
#include "common/error.h"
#include "common/random.h"
#include "common/EventRecorder.h"

#include "engines/advancedDetector.h"
#include "engines/util.h"

// Audio
#include "audio/mixer.h"

// AGS subsystems
#include "ags/ags.h"
#include "ags/constants.h"
#include "ags/resourceman.h"
#include "ags/sprites.h"
#include "ags/gamefile.h"

namespace AGS {

AGSEngine::AGSEngine(OSystem *syst, const AGSGameDescription *gameDesc) :
	Engine(syst), _gameDescription(gameDesc), _engineStartTime(0), _playTime(0),
	_width(0), _height(0), _resourceMan(0), _forceLetterbox(false), _needsUpdate(true),
	_mouseFrame(0), _mouseDelay(0) {

	_rnd = new Common::RandomSource("ags");
}

AGSEngine::~AGSEngine() {
	delete _gameFile;
	delete _resourceMan;

	delete _rnd;
}

Common::Error AGSEngine::run() {
	if (!init())
		return Common::kUnknownError;

	setCursorMode(MODE_WALK);
	// FIXME: filter->setMousePosition(160, 100);
	// FIXME: newMusic(0);

	return Common::kNoError;
}

uint32 AGSEngine::getGameFileVersion() const {
	return _gameFile->_version;
}

uint32 AGSEngine::getGUIVersion() const {
	return _gameFile->_guiVersion;
}

Common::SeekableReadStream *AGSEngine::getFile(const Common::String &filename) const {
	return _resourceMan->getFile(filename);
}

Common::String AGSEngine::getMasterArchive() const {
	const ADGameFileDescription *gameFiles = getGameFiles();

	if (gameFiles[0].fileName)
		return gameFiles[0].fileName;

	return "ac2game.dat";
}

bool AGSEngine::init() {
	// Open the archive file
	_resourceMan = new ResourceManager();
	if (!_resourceMan->init(getMasterArchive()))
		return false;

	// Load the game file
	_gameFile = new GameFile(this);
	if (!_gameFile->init())
		return false;

	Common::SeekableReadStream *spritesStream = getFile("acsprset.spr");
	if (!spritesStream)
		return false;
	_sprites = new SpriteSet(this, spritesStream);

	// Init graphics

	if (!getScreenSize())
		return false;

	if (!initGraphics())
		return false;

	syncSoundSettings();

	_engineStartTime = g_system->getMillis();

	return true;
}

bool AGSEngine::getScreenSize() {
	_baseWidth = 320;
	_baseHeight = 200;
	_textMultiply = 2;

	switch (_gameFile->_defaultResolution) {
	case 0:
		_textMultiply = 1;
		break;
	case 1:
	case 2:
		_width = 640;
		_height = 480;
		break;
	case 3:
	case 4:
		_width = 640;
		_height = 400;
		break;
	case 5:
		_baseWidth = 400;
		_baseHeight = 300;
		break;
	default:
		_baseWidth = 512;
		_baseHeight = 384;
		break;
	}

	if (_gameFile->_defaultResolution >= 5) {
		_width = _baseWidth * 2;
		_height = _baseHeight * 2;
		_gameFile->_options[OPT_LETTERBOX] = 0;
		_forceLetterbox = false;
	}

	return true;
}

bool AGSEngine::initGraphics() {
	if ((_width == 0) || (_height == 0))
		return false;

	::initGraphics(_width, _height, _width != 320, 0);

	return true;
}

void AGSEngine::pauseEngineIntern(bool pause) {
	_mixer->pauseAll(pause);
}

// reset the visible cursor to the one for the current mode
void AGSEngine::setDefaultCursor() {
	setMouseCursor(_cursorMode);
}

// TODO: rename to something which indicates it sets too?
uint AGSEngine::findNextEnabledCursor(uint32 startWith) {
	if (startWith >= _gameFile->_cursors.size())
		startWith = 0;

	// loop through all cursors until we find one we can use
	uint32 testing = startWith;
	do {
		if (!(_gameFile->_cursors[testing]._flags & MCF_DISABLED)) {
			if (testing == MODE_USE) {
				// inventory cursor - use it if the player has an active item
				// FIXME: inventory logic
				// if (_playerChar->_activeInv != 0xffffffff)
				// 	break;
			} else if (_gameFile->_cursors[testing]._flags & MCF_STANDARD) {
				// standard enabled cursor - use this one
				break;
			}
		}

		if (++testing >= _gameFile->_cursors.size())
			testing = 0;
	} while (testing != startWith);

	if (testing != startWith)
		setCursorMode(testing);

	return testing;
}

void AGSEngine::setCursorMode(uint32 newMode) {
	if (newMode >= _gameFile->_cursors.size())
		error("setCursorMode: invalid cursor mode %d (only %d cursors)", newMode, _gameFile->_cursors.size());
	_needsUpdate = true;

	if (_gameFile->_cursors[newMode]._flags & MCF_DISABLED) {
		findNextEnabledCursor(newMode);
		return;
	}

	if (newMode == MODE_USE) {
		// FIXME: inventory logic
		// if (_playerChar->_activeInv == 0xffffffff) {
		// 	findNextEnabledCursor(0);
		// 	return;
		// }
		// updateInvCursor(_playerChar->_activeInv);
	}

	_cursorMode = newMode;
	setDefaultCursor();

	debug(1, "cursor mode set to %d", newMode);
}

void AGSEngine::setMouseCursor(uint32 cursor) {
	assert(cursor < _gameFile->_cursors.size());

	MouseCursor &cursorInfo = _gameFile->_cursors[cursor];

	setCursorGraphic(cursorInfo._pic);
	/* FIXME if (_dottedMouseCursor) {
		delete _dottedMouseCursor;
		_dottedMouseCursor = NULL;
	} */

	if ((cursor == MODE_USE) && cursorInfo._pic && (_gameFile->_hotDot || _gameFile->_invHotDotSprite)) {
		// create a copy of cursor with the hotspot dot onto it, if needed
		/* FIXME: duplicate active mouse cursor onto _dottedMouseCursor */

		if (_gameFile->_invHotDotSprite) {
			// FIXME: draw invHotDotSprite centered on hotspot
		} else {
			// FIXME: draw pixel
			if (_gameFile->_hotDotOuter) {
				// FIXME: draw pixels
			}
		}

		// FIXME: replace active mouse cursor with _dottedMouseCursor
		updateCachedMouseCursor();
	}

	mouseSetHotspot(cursorInfo._hotspotX, cursorInfo._hotspotY);

	if (cursor != _currentCursor) {
		_currentCursor = cursor;
		_mouseFrame = 0;
		_mouseDelay = 0;
	}
}

void AGSEngine::setCursorGraphic(uint32 spriteId) {
	_cursorSprite = _sprites->getSprite(spriteId);

	if (!spriteId || !_cursorSprite) {
		// FIXME
	}

	_alphaBlendCursor = (bool)(_gameFile->_spriteFlags[spriteId] & SPF_ALPHACHANNEL);

	updateCachedMouseCursor();
}

void AGSEngine::updateCachedMouseCursor() {
	// FIXME: set the mouse cursor
}

void AGSEngine::mouseSetHotspot(uint32 x, uint32 y) {
	// FIXME: set the hotspot!
}

} // End of namespace AGS
