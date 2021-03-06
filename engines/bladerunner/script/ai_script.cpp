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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "bladerunner/script/ai_script.h"

#include "bladerunner/bladerunner.h"

#include "bladerunner/actor.h"

namespace BladeRunner {

AIScripts::AIScripts(BladeRunnerEngine *vm, int actorCount) {
	_vm = vm;
	_inScriptCounter = 0;
	_actorCount = actorCount;
	_actorUpdating = new bool[actorCount];
	_AIScripts = new AIScriptBase*[actorCount];
	for (int i = 0; i < actorCount; ++i) {
		_AIScripts[i] = nullptr;
		_actorUpdating[i] = false;
	}

	_AIScripts[kActorMcCoy] = new AIScriptMcCoy(_vm);
	_AIScripts[kActorRunciter] = new AIScriptRunciter(_vm);
	_AIScripts[kActorOfficerLeary] = new AIScriptOfficerLeary(_vm);
	_AIScripts[kActorLeon] = new AIScriptLeon(_vm);
	_AIScripts[kActorMaggie] = new AIScriptMaggie(_vm);
}

AIScripts::~AIScripts() {
	for (int i = 0; i < _actorCount; ++i) {
		delete _AIScripts[i];
		_AIScripts[i] = nullptr;
	}
	delete[] _AIScripts;
	delete[] _actorUpdating;
}

void AIScripts::initialize(int actor) {
	assert(actor < _actorCount);
	if (_AIScripts[actor]) {
		_AIScripts[actor]->Initialize();
	}
}

void AIScripts::update(int actor) {
	assert(actor < _actorCount);
	if (this->_actorUpdating[actor] != 1) {
		this->_actorUpdating[actor] = true;
		++this->_inScriptCounter;
		if (_AIScripts[actor])
			_AIScripts[actor]->Update();
		--this->_inScriptCounter;
		this->_actorUpdating[actor] = false;
	}
}

void AIScripts::timerExpired(int actor, int timer) {
	assert(actor < _actorCount);
	_inScriptCounter++;
	if (_AIScripts[actor]) {
		_AIScripts[actor]->TimerExpired(timer);
	}
	_inScriptCounter--;
}

void AIScripts::completedMovementTrack(int actor) {
	assert(actor < _actorCount);
	if (!_vm->_actors[actor]->inCombat()) {
		_inScriptCounter++;
		if (_AIScripts[actor]) {
			_AIScripts[actor]->CompletedMovementTrack();
		}
		_inScriptCounter--;
	}
}

void AIScripts::receivedClue(int actor, int clueId, int fromActorId) {
	assert(actor < _actorCount);
	_inScriptCounter++;
	if (_AIScripts[actor]) {
		_AIScripts[actor]->ReceivedClue(clueId, fromActorId);
	}
	_inScriptCounter--;
}

void AIScripts::clickedByPlayer(int actor) {
	assert(actor < _actorCount);

	if(_vm->_actors[actor]->inCombat()) {
		return;
	}

	_inScriptCounter++;
	if (_AIScripts[actor]) {
		_AIScripts[actor]->ClickedByPlayer();
	}
	_inScriptCounter--;
}

void AIScripts::enteredScene(int actor, int setId) {
	assert(actor < _actorCount);
	_inScriptCounter++;
	if (_AIScripts[actor]) {
		_AIScripts[actor]->EnteredScene(setId);
	}
	_inScriptCounter--;
}

void AIScripts::otherAgentEnteredThisScene(int actor, int otherActorId) {
	assert(actor < _actorCount);
	_inScriptCounter++;
	if (_AIScripts[actor]) {
		_AIScripts[actor]->OtherAgentEnteredThisScene(otherActorId);
	}
	_inScriptCounter--;
}

void AIScripts::otherAgentExitedThisScene(int actor, int otherActorId) {
	assert(actor < _actorCount);
	_inScriptCounter++;
	if (_AIScripts[actor]) {
		_AIScripts[actor]->OtherAgentExitedThisScene(otherActorId);
	}
	_inScriptCounter--;
}

void AIScripts::retired(int actor, int retiredByActorId) {
	assert(actor < _actorCount);
	_inScriptCounter++;
	if (_AIScripts[actor]) {
		_AIScripts[actor]->Retired(retiredByActorId);
	}
	_inScriptCounter--;
}

void AIScripts::goalChanged(int actor, int currentGoalNumber, int newGoalNumber) {
	assert(actor < _actorCount);
	_inScriptCounter++;
	if (_AIScripts[actor]) {
		_AIScripts[actor]->GoalChanged(currentGoalNumber, newGoalNumber);
	}
	_inScriptCounter--;
}

bool AIScripts::reachedMovementTrackWaypoint(int actor, int waypointId) {
	assert(actor < _actorCount);
	bool result = false;
	if (!_vm->_actors[actor]->inCombat()) {
		_inScriptCounter++;
		if (_AIScripts[actor]) {
			result = _AIScripts[actor]->ReachedMovementTrackWaypoint(waypointId);
		}
		_inScriptCounter--;
	}
	return result;
}

void AIScripts::updateAnimation(int actor, int *animation, int *frame) {
	assert(actor < _actorCount);
	_inScriptCounter++;
	if (_AIScripts[actor]) {
		_AIScripts[actor]->UpdateAnimation(animation, frame);
	}
	_inScriptCounter--;
}

void AIScripts::changeAnimationMode(int actor, int mode) {
	assert(actor < _actorCount);
	_inScriptCounter++;
	if (_AIScripts[actor]) {
		_AIScripts[actor]->ChangeAnimationMode(mode);
	}
	_inScriptCounter--;
}

} // End of namespace BladeRunner
