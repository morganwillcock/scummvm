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
 */

#include "fullpipe/fullpipe.h"

#include "common/file.h"
#include "common/array.h"
#include "common/list.h"

#include "fullpipe/objects.h"
#include "fullpipe/gameobj.h"

namespace Fullpipe {

bool FullpipeEngine::loadGam(const char *fname) {
	_gameLoader = new CGameLoader();

	if (_gameLoader->loadFile(fname)) {
		g_fullpipe->_currSoundListCount = 0;
		initObjectStates();

		//set_g_messageQueueCallback1(messageQueueCallback1);
		//addMessageHandler2(global_messageHandler, 0, 4);
		_inventory = &_gameLoader->_inventory;
		_inventory->setItemFlags(ANI_INV_MAP, 0x10003);
		_inventory->addItem(ANI_INV_MAP, 1);

#if 1
		g_fullpipe->accessScene(301);
		g_fullpipe->accessScene(302);
		g_fullpipe->accessScene(303);
		g_fullpipe->accessScene(304);
		g_fullpipe->accessScene(305);
		g_fullpipe->accessScene(321);
		g_fullpipe->accessScene(635);
		g_fullpipe->accessScene(649);
		g_fullpipe->accessScene(650);
		g_fullpipe->accessScene(651);
		g_fullpipe->accessScene(652);
		g_fullpipe->accessScene(653);
		g_fullpipe->accessScene(654);
		g_fullpipe->accessScene(655);
		g_fullpipe->accessScene(726);
		g_fullpipe->accessScene(858);
		g_fullpipe->accessScene(903);
		g_fullpipe->accessScene(1137);
		g_fullpipe->accessScene(1138);
		g_fullpipe->accessScene(1139);
		g_fullpipe->accessScene(1140);
		g_fullpipe->accessScene(1141);
		g_fullpipe->accessScene(1142);
		g_fullpipe->accessScene(1143);
		g_fullpipe->accessScene(1144);
		g_fullpipe->accessScene(1546);
		g_fullpipe->accessScene(1547);
		g_fullpipe->accessScene(1548);
		g_fullpipe->accessScene(1549);
		g_fullpipe->accessScene(1550);
		g_fullpipe->accessScene(1551);
		g_fullpipe->accessScene(1552);
		g_fullpipe->accessScene(2062);
		g_fullpipe->accessScene(2063);
		g_fullpipe->accessScene(2064);
		g_fullpipe->accessScene(2065);
		g_fullpipe->accessScene(2066);
		g_fullpipe->accessScene(2067);
		g_fullpipe->accessScene(2068);
		g_fullpipe->accessScene(2069);
		g_fullpipe->accessScene(2070);
		g_fullpipe->accessScene(2071);
		g_fullpipe->accessScene(2072);
		g_fullpipe->accessScene(2460);
		g_fullpipe->accessScene(3896);
		g_fullpipe->accessScene(3907);
		g_fullpipe->accessScene(4620);
		g_fullpipe->accessScene(4999);
		g_fullpipe->accessScene(5000);
		g_fullpipe->accessScene(5001);
		g_fullpipe->accessScene(5166);
		g_fullpipe->accessScene(5222);
#endif

		_inventory->rebuildItemRects();

		//for (CPtrList::iterator s = _inventory->getScene()->_picObjList.begin(); s != _inventory->getScene()->_picObjList.end(); ++s) {
		//}

		//_sceneSwitcher = sceneSwitcher;
		//_preloadCallback = gameLoaderPreloadCallback
		//_readSavegameCallback = gameLoaderReadSavegameCallback;
		_aniMan = accessScene(SC_COMMON)->getAniMan();
		_scene2 = 0;
		
		warning("STUB: loadGam()");
	} else
		return false;

	return true;
}

CGameLoader::CGameLoader() {
	_interactionController = new CInteractionController();

	_gameProject = 0;
	//_gameName = "untitled";

	//addMessageHandler2(CGameLoader_messageHandler1, 0, 0);
	//insertMessageHandler(CGameLoader_messageHandler2, 0, 128);
	//insertMessageHandler(CGameLoader_messageHandler3, 0, 1);

	_field_FA = 0;
	_field_F8 = 0;
	_sceneSwitcher = 0;
	_preloadCallback = 0;
	_readSavegameCallback = 0;
	_gameVar = 0;
	_preloadId1 = 0;
	_preloadId2 = 0;
	_updateCounter = 0;

	//g_x = 0;
	//g_y = 0;
	//dword_478480 = 0;
	//g_objectId2 = 0;
	//g_id = 0;
}

CGameLoader::~CGameLoader() {
	free(_gameName);
	delete _gameProject;
}

bool CGameLoader::load(MfcArchive &file) {
	debug(5, "CGameLoader::load()");

	_gameName = file.readPascalString();
	debug(6, "_gameName: %s", _gameName);

	_gameProject = new GameProject();

	_gameProject->load(file);

	g_fullpipe->_gameProject = _gameProject;

	if (g_fullpipe->_gameProjectVersion < 12) {
		error("Old gameProjectVersion: %d", g_fullpipe->_gameProjectVersion);
	}

	_gameName = file.readPascalString();
	debug(6, "_gameName: %s", _gameName);

	_inventory.load(file);

	_interactionController->load(file);

	debug(6, "sceneTag count: %d", _gameProject->_sceneTagList->size());

	_sc2array.resize(_gameProject->_sceneTagList->size());

	int i = 0;
	for (SceneTagList::const_iterator it = _gameProject->_sceneTagList->begin(); it != _gameProject->_sceneTagList->end(); ++it, i++) {
		char tmp[12];

		snprintf(tmp, 11, "%04d.sc2", it->_sceneId);

		debug(2, "sc: %s", tmp);

		_sc2array[i].loadFile((const char *)tmp);
	}

	_preloadItems.load(file);

	_field_FA = file.readUint16LE();
	_field_F8 = file.readUint16LE();

	_gameVar = (CGameVar *)file.readClass();

	return true;
}

bool CGameLoader::loadScene(int num) {
	SceneTag *st;

	int idx = getSceneTagBySceneId(num, &st);

	if (st->_scene)
		st->loadScene();

	if (st->_scene) {
		st->_scene->init();

		applyPicAniInfos(st->_scene, _sc2array[idx]._defPicAniInfos, _sc2array[idx]._defPicAniInfosCount);
		applyPicAniInfos(st->_scene, _sc2array[idx]._picAniInfos, _sc2array[idx]._picAniInfosCount);

		_sc2array[idx]._scene = st->_scene;
		_sc2array[idx]._isLoaded = 1;

		return true;
	}

	return false;
}

int CGameLoader::getSceneTagBySceneId(int num, SceneTag **st) {
	warning("STUB: CGameLoader::getSceneTagBySceneId()");

	return 0;
}

void CGameLoader::applyPicAniInfos(Scene *sc, PicAniInfo **picAniInfo, int picAniInfoCount) {
	warning("STUB: CGameLoader::applyPicAniInfo()");
}

GameProject::GameProject() {
	_field_4 = 0;
	_headerFilename = 0;
	_field_10 = 12;
}

bool GameProject::load(MfcArchive &file) {
	debug(5, "GameProject::load()");

	_field_4 = 0;
	_headerFilename = 0;
	_field_10 = 12;

	g_fullpipe->_gameProjectVersion = file.readUint32LE();
	g_fullpipe->_pictureScale = file.readUint16LE();
	g_fullpipe->_scrollSpeed = file.readUint32LE();

	_headerFilename = file.readPascalString();

	debug(1, "_gameProjectVersion = %d", g_fullpipe->_gameProjectVersion);
	debug(1, "_pictureScale = %d", g_fullpipe->_pictureScale);
	debug(1, "_scrollSpeed = %d", g_fullpipe->_scrollSpeed);
	debug(1, "_headerFilename = %s", _headerFilename);

	_sceneTagList = new SceneTagList();

	_sceneTagList->load(file);

	if (g_fullpipe->_gameProjectVersion >= 3)
		_field_4 = file.readUint32LE();

	if (g_fullpipe->_gameProjectVersion >= 5) {
		file.readUint32LE();
		file.readUint32LE();
	}

	return true;
}

GameProject::~GameProject() {
	free(_headerFilename);
}

bool CInteractionController::load(MfcArchive &file) {
	debug(5, "CInteractionController::load()");

	return _interactions.load(file);
}

CInputController::CInputController() {
	// TODO
}

CInteraction::CInteraction() {
	_objectId1 = 0;
	_objectId2 = 0;
	_staticsId1 = 0;
	_objectId3 = 0;
	_objectState2 = 0;
	_objectState1 = 0;
	_messageQueue = 0;
	_flags = 0;
	_yOffs = 0;
	_xOffs = 0;
	_staticsId2 = 0;
	_field_28 = 0;
	_sceneId = -1;
}

bool CInteraction::load(MfcArchive &file) {
	debug(5, "CInteraction::load()");

	_objectId1 = file.readUint16LE();
	_objectId2 = file.readUint16LE();
	_staticsId1 = file.readUint16LE();
	_staticsId2 = file.readUint16LE();
	_objectId3 = file.readUint16LE();
	_objectState2 = file.readUint32LE();
	_objectState1 = file.readUint32LE();
	_xOffs = file.readUint32LE();
	_yOffs = file.readUint32LE();
	_sceneId = file.readUint32LE();
	_flags = file.readUint32LE();
	_actionName = file.readPascalString();

	_messageQueue = (MessageQueue *)file.readClass();

	return true;
}

ExCommand::ExCommand() {
	_field_3C = 1;
	_messageNum = 0;
	_flags = 0;
	_parId = 0;
}

bool ExCommand::load(MfcArchive &file) {
	debug(5, "ExCommand::load()");

	_msg._parentId = file.readUint16LE();
	_msg._messageKind = file.readUint32LE();
	_msg._x = file.readUint32LE();
	_msg._y = file.readUint32LE();
	_msg._field_14 = file.readUint32LE();
	_msg._sceneClickX = file.readUint32LE();
	_msg._sceneClickY = file.readUint32LE();
	_msg._field_20 = file.readUint32LE();
	_msg._field_24 = file.readUint32LE();
	_msg._param28 = file.readUint32LE();
	_msg._field_2C = file.readUint32LE();
	_msg._field_30 = file.readUint32LE();
	_msg._field_34 = file.readUint32LE();

	_messageNum = file.readUint32LE();

	_field_3C = 0;

	if (g_fullpipe->_gameProjectVersion >= 12) {
		_flags = file.readUint32LE();
		_parId = file.readUint32LE();
	}

	return true;
}

Message::Message() {
	_messageKind = 0;
	_parentId = 0;
	_x = 0;
	_y = 0;
	_field_14 = 0;
	_sceneClickX = 0;
	_sceneClickY = 0;
	_field_20 = 0;
	_field_24 = 0;
	_param28 = 0;
	_field_2C = 0;
	_field_30 = 0;
	_field_34 = 0;
}

CObjstateCommand::CObjstateCommand() {
	_value = 0;
}

bool CObjstateCommand::load(MfcArchive &file) {
	debug(5, "CObjStateCommand::load()");

	_cmd.load(file);

	_value = file.readUint32LE();

	_objCommandName = file.readPascalString();

	return true;
}

bool PreloadItems::load(MfcArchive &file) {
	debug(5, "PreloadItems::load()");

	int count = file.readCount();

	resize(count);

	for (int i = 0; i < count; i++) {
		PreloadItem *t = new PreloadItem();
		t->preloadId1 = file.readUint32LE();
		t->preloadId2 = file.readUint32LE();
		t->sceneId = file.readUint32LE();
		t->field_C = file.readUint32LE();

		push_back(*t);
	}

	return true;
}

CGameVar::CGameVar() {
	_subVars = 0;
	_parentVarObj = 0;
	_nextVarObj = 0;
	_prevVarObj = 0;
	_field_14 = 0;
	_varType = 0;
	_value.floatValue = 0;
}

bool CGameVar::load(MfcArchive &file) {
	_varName = file.readPascalString();
	_varType = file.readUint32LE();

	debugN(6, "[%03d] ", file.getLevel());
	for (int i = 0; i < file.getLevel(); i++)
		debugN(6, " ");

	debugN(6, "<%s>: ", transCyrillic((byte *)_varName));

	switch (_varType) {
	case 0:
		_value.intValue = file.readUint32LE();
		debug(6, "d --> %d", _value.intValue);
		break;
	case 1:
		_value.intValue = file.readUint32LE(); // FIXME
		debug(6, "f --> %f", _value.floatValue);
		break;
	case 2:
		_value.stringValue = file.readPascalString();
		debug(6, "s --> %s", _value.stringValue);
		break;
	default:
		error("Unknown var type: %d (0x%x)", _varType, _varType);
	}

	file.incLevel();
	_parentVarObj = (CGameVar *)file.readClass();
	_prevVarObj = (CGameVar *)file.readClass();
	_nextVarObj = (CGameVar *)file.readClass();
	_field_14 = (CGameVar *)file.readClass();
	_subVars = (CGameVar *)file.readClass();
	file.decLevel();

	return true;
}

CGameVar *CGameVar::getSubVarByName(const char *name) {
	CGameVar *sv = 0;

	if (_subVars != 0) {
		sv = _subVars;
		for (;sv && scumm_stricmp(sv->_varName, name); sv = sv->_nextVarObj)
			;
	}
	return sv;
}

bool CGameVar::setSubVarAsInt(const char *name, int value) {
	CGameVar *var = getSubVarByName(name);

	if (var) {
		if (var->_varType == 0) {
			var->_value.intValue = value;

			return true;
		}
		return false;
	}

	var = new CGameVar();
	var->_varType = 0;
	var->_value.intValue = value;
	var->_varName = (char *)calloc(strlen(name) + 1, 1);
	strcpy(var->_varName, name);

	return addSubVar(var);
}

int CGameVar::getSubVarAsInt(const char *name) {
	CGameVar *var = getSubVarByName(name);

	if (var)
		return var->_value.intValue;
	else
		return 0;
}

CGameVar *CGameVar::addSubVarAsInt(const char *name, int value) {
	if (getSubVarByName(name)) {
		return 0;
	} else {
		CGameVar *var = new CGameVar();

		var->_varType = 0;
		var->_value.intValue = value;

		var->_varName = (char *)calloc(strlen(name) + 1, 1);
		strcpy(var->_varName, name);

		return (addSubVar(var) != 0) ? var : 0;
	}
}

bool CGameVar::addSubVar(CGameVar *subvar) {
	CGameVar *var = _subVars;

	if (var) {
		for (CGameVar *i = var->_nextVarObj; i; i = i->_nextVarObj)
			var = i;

		var->_nextVarObj = subvar;
		subvar->_prevVarObj = var;
		subvar->_parentVarObj = var;

		return true;
	} else {
		var->_subVars = subvar;
		subvar->_parentVarObj = var;

		return true;
	}

	return false;
}

Sc2::Sc2() {
	_sceneId = 0;
	_field_2 = 0;
	_scene = 0;
	_motionController = 0;
	_data1 = 0;
	_count1 = 0;
	_defPicAniInfos = 0;
	_defPicAniInfosCount = 0;
	_picAniInfos = 0;
	_picAniInfosCount = 0;
	_isLoaded = 0;
	_entranceData = 0;
	_entranceDataCount = 0;
}

bool Sc2::load(MfcArchive &file) {
	debug(5, "Sc2::load()");

	_sceneId = file.readUint16LE();

	_motionController = (CMotionController *)file.readClass();

	_count1 = file.readUint32LE();
	debug(4, "count1: %d", _count1);
	if (_count1 > 0) {
		_data1 = (int32 *)malloc(_count1 * sizeof(int32));

		for (int i = 0; i < _count1; i++) {
			_data1[i] = file.readUint32LE();
		}
	} else {
		_data1 = 0;
	}

	_defPicAniInfosCount = file.readUint32LE();
	debug(4, "defPicAniInfos: %d", _defPicAniInfosCount);
	if (_defPicAniInfosCount > 0) {
		_defPicAniInfos = (PicAniInfo **)malloc(_defPicAniInfosCount * sizeof(PicAniInfo *));

		for (int i = 0; i < _defPicAniInfosCount; i++) {
			_defPicAniInfos[i] = new PicAniInfo();

			_defPicAniInfos[i]->load(file);
		}
	} else {
		_defPicAniInfos = 0;
	}

	_picAniInfos = 0;
	_picAniInfosCount = 0;

	_entranceDataCount = file.readUint32LE();
	debug(4, "_entranceData: %d", _entranceDataCount);

	if (_entranceDataCount > 0) {
		_entranceData = (EntranceInfo **)malloc(_defPicAniInfosCount * sizeof(EntranceInfo *));

		for (int i = 0; i < _entranceDataCount; i++) {
			_entranceData[i] = new EntranceInfo();
			_entranceData[i]->load(file);
		}
	} else {
		_entranceData = 0;
	}

	debug(4, "pos: %d, 0x%x: %d", file.size(), file.pos(), file.size() - file.pos());

	return true;
}

bool PicAniInfo::load(MfcArchive &file) {
	debug(5, "PicAniInfo::load()");

	type = file.readUint32LE();
	objectId = file.readUint16LE();
	field_6 = file.readUint16LE();
	field_8 = file.readUint32LE();
	field_C = file.readUint16LE();
	field_E = file.readUint16LE();
	ox = file.readUint32LE();
	oy = file.readUint32LE();
	priority = file.readUint32LE();
	staticsId = file.readUint16LE();
	movementId = file.readUint16LE();
	dynamicPhaseIndex = file.readUint16LE();
	flags = file.readUint16LE();
	field_24 = file.readUint32LE();
	someDynamicPhaseIndex = file.readUint32LE();

	return true;
}

bool EntranceInfo::load(MfcArchive &file) {
	debug(5, "EntranceInfo::load()");

	sceneId = file.readUint32LE();
	field_4 = file.readUint32LE();
	messageQueueId = file.readUint32LE();
	file.read(gap_C, 292); // FIXME, Ugh
	field_130 = file.readUint32LE();

	return true;
}

} // End of namespace Fullpipe
