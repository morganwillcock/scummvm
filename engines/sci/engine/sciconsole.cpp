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

/* Second half of the console implementation: VM dependent stuff */
/* Remember, it doesn't have to be fast. */

#include "sci/engine/state.h"
#include "sci/engine/sciconsole.h"

#include "sci/sci.h"	// For _console only
#include "sci/console.h"	// For _console only

namespace Sci {

// console commands

static int c_list(EngineState *s, const Common::Array<cmd_param_t> &cmdParams); // lists various types of things
static int c_set(EngineState *s, const Common::Array<cmd_param_t> &cmdParams); // sets an int variable
static int c_print(EngineState *s, const Common::Array<cmd_param_t> &cmdParams); // prints a variable
//static int c_objinfo(EngineState *s, const Common::Array<cmd_param_t> &cmdParams); // shows some info about one class
//static int c_objmethods(EngineState *s, const Common::Array<cmd_param_t> &cmdParams); // Disassembles all methods of a class
static int c_hexgrep(EngineState *s, const Common::Array<cmd_param_t> &cmdParams); // Searches a string in one resource or resource class

struct cmd_mm_entry_t {
	const char *name;
	const char *description;
}; // All later structures must "extend" this

// Simple info page
struct cmd_page_t : public cmd_mm_entry_t {
};

struct cmd_command_t : public cmd_mm_entry_t {
	ConCommand command;
	const char *param;
};

struct cmd_var_t : public cmd_mm_entry_t {
	union {
		int *intp;
		char **charpp;
		reg_t *reg;
	} var;
};


typedef void printfunc_t(cmd_mm_entry_t *data, int full);

struct cmd_mm_struct_t {
	const char *name;
	void *data; // cmd_mm_entry_t
	size_t size_per_entry;
	printfunc_t *print;
	int entries; // Number of used entries
	int allocated;  // Number of allocated entries
};

#define CMD_MM_ENTRIES 3 // command console memory and manual page manager
#define CMD_MM_DEFAULT_ALLOC 4 // Number of table entries to allocate per default

#define CMD_MM_CMD 0 // Commands
#define CMD_MM_VAR 1 // Variables
#define CMD_MM_DOC 2 // Misc. documentation

static const char *cmd_mm_names[CMD_MM_ENTRIES] = {
	"Commands",
	"Variables",
	"Documentation"
};
static size_t cmd_mm_sizes_per_entry[CMD_MM_ENTRIES] = {
	sizeof(cmd_command_t),
	sizeof(cmd_var_t),
	sizeof(cmd_page_t)
};

static void _cmd_print_command(cmd_mm_entry_t *data, int full);
static void _cmd_print_var(cmd_mm_entry_t *data, int full);
static void _cmd_print_page(cmd_mm_entry_t *data, int full);

static printfunc_t *cmd_mm_printers[CMD_MM_ENTRIES] = {
	_cmd_print_command,
	_cmd_print_var,
	_cmd_print_page
};

static cmd_mm_struct_t cmd_mm[CMD_MM_ENTRIES];

static int _cmd_initialized = 0;
static int _lists_need_sorting = 0;

void _cmd_exit() {
	int t;

	for (t = 0; t < CMD_MM_ENTRIES; t++)
		free(cmd_mm[t].data);
}

static cmd_mm_entry_t *cmd_mm_find(const char *name, int type) {
	int i;

	for (i = 0; i < cmd_mm[type].entries; i++) {
		cmd_mm_entry_t *tmp = (cmd_mm_entry_t *)((byte *)cmd_mm[type].data + i * cmd_mm[type].size_per_entry);
		if (!strcmp(tmp->name, name))
			return tmp;
	}

	return NULL;
}

static int _cmd_mm_comp(const void *a, const void *b) {
	return strcmp(((cmd_mm_entry_t *) a)->name, ((cmd_mm_entry_t *) b)->name);
}

void con_sort_all() {
	int i;

	for (i = 0; i < CMD_MM_ENTRIES; i++)
		if (cmd_mm[i].entries && _lists_need_sorting & (1 << i))
			qsort(cmd_mm[i].data, cmd_mm[i].entries, cmd_mm[i].size_per_entry, _cmd_mm_comp);

	_lists_need_sorting = 0;
}

void con_init() {
	if (!_cmd_initialized) {
		int i;

		_cmd_initialized = 1;
		for (i = 0; i < CMD_MM_ENTRIES; i++) {
			cmd_mm[i].name = cmd_mm_names[i];
			cmd_mm[i].size_per_entry = cmd_mm_sizes_per_entry[i];
			cmd_mm[i].entries = 0;
			cmd_mm[i].allocated = CMD_MM_DEFAULT_ALLOC;
			cmd_mm[i].data = calloc(cmd_mm[i].allocated, cmd_mm[i].size_per_entry);
			cmd_mm[i].print = cmd_mm_printers[i];
		}

		atexit(_cmd_exit);

		// Hook up some commands
		con_hook_command(&c_list, "list", "s*", "Lists various things (try 'list')");
		con_hook_command(&c_print, "print", "s", "Prints an int variable");
		con_hook_command(&c_set, "set", "si", "Sets an int variable");
		con_hook_command(&c_hexgrep, "hexgrep", "shh*", "Searches some resources for a\n"
		                 "  particular sequence of bytes, re-\n  presented as hexadecimal numbers.\n\n"
		                 "EXAMPLES:\n  hexgrep script e8 03 c8 00\n  hexgrep pic.042 fe");

		con_hook_page("addresses", "Passing address parameters\n\n"
		              "  Address parameters may be passed in one of\n"
		              "  three forms:\n"
		              "  - ssss:oooo -- where 'ssss' denotes a\n"
		              "    segment and 'oooo' an offset. Example:\n"
		              "    \"a:c5\" would address something in seg-\n"
		              "    ment 0xa at offset 0xc5.\n"
		              "  - &scr:oooo -- where 'scr' is a script number\n"
		              "    and oooo an offset within that script; will\n"
		              "    fail if the script is not currently loaded\n"
		              "  - $REG -- where 'REG' is one of 'PC', 'ACC',\n"
		              "    'PREV' or 'OBJ': References the address\n"
		              "    indicated by the register of this name.\n"
		              "  - $REG+n (or -n) -- Like $REG, but modifies\n"
		              "    the offset part by a specific amount (which\n"
		              "    is specified in hexadecimal).\n"
		              "  - ?obj -- Looks up an object with the specified\n"
		              "    name, uses its address. This will abort if\n"
		              "    the object name is ambiguous; in that case,\n"
		              "    a list of addresses and indices is provided.\n"
		              "    ?obj.idx may be used to disambiguate 'obj'\n"
		              "    by the index 'idx'.\n");

			// These were in sci.cpp
			/*
			con_hook_int(&(gfx_options.buffer_pics_nr), "buffer_pics_nr",
				"Number of pics to buffer in LRU storage\n");
			con_hook_int(&(gfx_options.pic0_dither_mode), "pic0_dither_mode",
				"Mode to use for pic0 dithering\n");
			con_hook_int(&(gfx_options.pic0_dither_pattern), "pic0_dither_pattern",
				"Pattern to use for pic0 dithering\n");
			con_hook_int(&(gfx_options.pic0_unscaled), "pic0_unscaled",
				"Whether pic0 should be drawn unscaled\n");
			con_hook_int(&(gfx_options.dirty_frames), "dirty_frames",
				"Dirty frames management\n");
			*/
	}
}

static inline int clone_is_used(CloneTable *t, int idx) {
	return t->isValidEntry(idx);
}

int parse_reg_t(EngineState *s, const char *str, reg_t *dest) { // Returns 0 on success
	int rel_offsetting = 0;
	const char *offsetting = NULL;
	// Non-NULL: Parse end of string for relative offsets
	char *endptr;

	if (!s) {
		sciprintf("Addresses can only be parsed if a global state is present");
		return 1; // Requires a valid state
	}

	if (*str == '$') { // Register
		rel_offsetting = 1;

		if (!scumm_strnicmp(str + 1, "PC", 2)) {
			*dest = s->_executionStack.back().addr.pc;
			offsetting = str + 3;
		} else if (!scumm_strnicmp(str + 1, "P", 1)) {
			*dest = s->_executionStack.back().addr.pc;
			offsetting = str + 2;
		} else if (!scumm_strnicmp(str + 1, "PREV", 4)) {
			*dest = s->r_prev;
			offsetting = str + 5;
		} else if (!scumm_strnicmp(str + 1, "ACC", 3)) {
			*dest = s->r_acc;
			offsetting = str + 4;
		} else if (!scumm_strnicmp(str + 1, "A", 1)) {
			*dest = s->r_acc;
			offsetting = str + 2;
		} else if (!scumm_strnicmp(str + 1, "OBJ", 3)) {
			*dest = s->_executionStack.back().objp;
			offsetting = str + 4;
		} else if (!scumm_strnicmp(str + 1, "O", 1)) {
			*dest = s->_executionStack.back().objp;
			offsetting = str + 2;
		} else
			return 1; // No matching register

		if (!*offsetting)
			offsetting = NULL;
		else if (*offsetting != '+' && *offsetting != '-')
			return 1;
	} else if (*str == '&') {
		int script_nr;
		// Look up by script ID
		char *colon = (char *)strchr(str, ':');

		if (!colon)
			return 1;
		*colon = 0;
		offsetting = colon + 1;

		script_nr = strtol(str + 1, &endptr, 10);

		if (*endptr)
			return 1;

		dest->segment = s->seg_manager->segGet(script_nr);

		if (!dest->segment) {
			return 1;
		}
	} else if (*str == '?') {
		int index = -1;
		int times_found = 0;
		char *tmp;
		const char *str_objname;
		char *str_suffix;
		char suffchar = 0;
		uint i;
		// Parse obj by name

		tmp = (char *)strchr(str, '+');
		str_suffix = (char *)strchr(str, '-');
		if (tmp < str_suffix)
			str_suffix = tmp;
		if (str_suffix) {
			suffchar = (*str_suffix);
			*str_suffix = 0;
		}

		tmp = (char *)strchr(str, '.');

		if (tmp) {
			*tmp = 0;
			index = strtol(tmp + 1, &endptr, 16);
			if (*endptr)
				return -1;
		}

		str_objname = str + 1;

		// Now all values are available; iterate over all objects.
		for (i = 0; i < s->seg_manager->_heap.size(); i++) {
			MemObject *mobj = s->seg_manager->_heap[i];
			int idx = 0;
			int max_index = 0;

			if (mobj) {
				if (mobj->getType() == MEM_OBJ_SCRIPT)
					max_index = (*(Script *)mobj)._objects.size();
				else if (mobj->getType() == MEM_OBJ_CLONES)
					max_index = (*(CloneTable *)mobj)._table.size();
			}

			while (idx < max_index) {
				int valid = 1;
				Object *obj = NULL;
				reg_t objpos;
				objpos.offset = 0;
				objpos.segment = i;

				if (mobj->getType() == MEM_OBJ_SCRIPT) {
					obj = &(*(Script *)mobj)._objects[idx];
					objpos.offset = obj->pos.offset;
				} else if (mobj->getType() == MEM_OBJ_CLONES) {
					obj = &((*(CloneTable *)mobj)._table[idx]);
					objpos.offset = idx;
					valid = clone_is_used((CloneTable *)mobj, idx);
				}

				if (valid) {
					const char *objname = obj_get_name(s, objpos);
					if (!strcmp(objname, str_objname)) {
						// Found a match!
						if ((index < 0) && (times_found > 0)) {
							if (times_found == 1) {
								// First time we realized the ambiguity
								sciprintf("Ambiguous:\n");
								sciprintf("  %3x: [%04x:%04x] %s\n", 0, PRINT_REG(*dest), str_objname);
							}
							sciprintf("  %3x: [%04x:%04x] %s\n", times_found, PRINT_REG(objpos), str_objname);
						}
						if (index < 0 || times_found == index)
							*dest = objpos;
						++times_found;
					}
				}
				++idx;
			}

		}

		if (!times_found)
			return 1;

		if (times_found > 1 && index < 0) {
			sciprintf("Ambiguous: Aborting.\n");
			return 1; // Ambiguous
		}

		if (times_found <= index)
			return 1; // Not found

		offsetting = str_suffix;
		if (offsetting)
			*str_suffix = suffchar;
		rel_offsetting = 1;
	} else {
		char *colon = (char *)strchr(str, ':');

		if (!colon) {
			offsetting = str;
			dest->segment = 0;
		} else {
			*colon = 0;
			offsetting = colon + 1;

			dest->segment = strtol(str, &endptr, 16);
			if (*endptr)
				return 1;
		}
	}
	if (offsetting) {
		int val = strtol(offsetting, &endptr, 16);

		if (rel_offsetting)
			dest->offset += val;
		else
			dest->offset = val;

		if (*endptr)
			return 1;
	}

	return 0;
}

void con_parse(EngineState *s, const char *command) {
	char *cmd = (command && command[0]) ? (char *)strdup(command) : (char *)strdup(" ");
	char *_cmd = cmd;
	int pos = 0;

	if (!_cmd_initialized)
		con_init();

	bool done = false;		// are we done yet?
	while (!done) {
		cmd_command_t *command_todo;
		bool quote = false;		// quoting?
		bool cdone = false;		// Done with the current command?
		bool onvar = true;		// currently working on a variable?
		cdone = 0;
		pos = 0;

		Common::Array<cmd_param_t> cmdParams;

		while (*cmd == ' ')
			cmd++;

		while (!cdone) {
			switch (cmd[pos]) {
			case 0:
				cdone = done = true;
			case ';':
				if (!quote)
					cdone = true;
			case ' ':
				if (!quote) {
					cmd[pos] = 0;
					onvar = false;
				}
				break;
			case '\\':		// don't check next char for special meaning
				memmove(cmd + pos, cmd + pos + 1, strlen(cmd + pos) - 1);
				break;
			case '"':
				quote = !quote;
				memmove(cmd + pos, cmd + pos + 1, strlen(cmd + pos));
				pos--;
				break;
			default:
				if (!onvar) {
					onvar = true;
					cmd_param_t tmp;
					tmp.str = cmd + pos;
					cmdParams.push_back(tmp);
				}
				break;
			}
			pos++;
		}

		if (quote)
			sciprintf("unbalanced quotes\n");
		else if (strcmp(cmd, "") != 0) {
			command_todo = (cmd_command_t *) cmd_mm_find(cmd, CMD_MM_CMD);
			if (!command_todo)
				sciprintf("%s: not found\n", cmd);
			else {
				uint minparams;
				int need_state = 0;

				const char *paramt = command_todo->param;	// parameter types
				if (command_todo->param[0] == '!') {
					need_state = 1;
					paramt++;
				}

				minparams = strlen(paramt);

				if ((paramt[0] != 0) && (paramt[strlen(paramt) - 1] == '*'))
					minparams -= 2;

				if (cmdParams.size() < minparams)
					sciprintf("%s: needs more than %d parameters\n", cmd, cmdParams.size());

				else if ((cmdParams.size() > strlen(paramt)) && ((strlen(paramt) == 0) || paramt[strlen(paramt) - 1] != '*'))
					sciprintf("%s: too many parameters", cmd);
				else {
					int do_execute = !need_state || s; // /me wants an implication arrow
					char paramtype;
					int paramtypepos = 0;
					char *endptr;

					for (uint i = 0; i < cmdParams.size(); i++) {
						paramtype = paramt[paramtypepos];

						if ((paramt[paramtypepos + 1]) && (paramt[paramtypepos + 1] != '*'))
							paramtypepos++;
						// seek next param type unless end of string or '*						   '

						switch (paramtype) {
							// Now turn the parameters into variables of the appropriate types,
							// unless they're strings, and store them into the cmdParams array

						case 'a': {
							const char *oldname = cmdParams[i].str;
							if (parse_reg_t(s, oldname, &(cmdParams[i].reg))) {
								sciprintf("%s: '%s' is not an address or object\n", cmd, oldname);
								do_execute = 0;
							}
							break;
						}

						case 'i': {
							const char *orgstr = cmdParams[i].str;

							cmdParams[i].val = strtol(orgstr, &endptr, 0);
							if (*endptr != '\0') {
								do_execute = 0;
								sciprintf("%s: '%s' is not an int\n", cmd, orgstr);
							}
						}
						break;

						case 'h': {
							const char *orgstr = cmdParams[i].str;

							cmdParams[i].val = strtol(orgstr, &endptr, 16);

							if (*endptr != '\0') {
								do_execute = 0;
								sciprintf("%s: '%s' is not a hex number\n", cmd, orgstr);
							}

							cmdParams[i].val &= 0xff;	// Clip hex numbers to 0x00 ... 0xff
						}
						break;

						case 's':
							break;

						default:
							warning("Internal error: Heap corruption or prior assertion failed: "
							        "Unknown parameter type '%c' for function", paramtype);

						}
					}

					if (do_execute) {
						command_todo->command(s, cmdParams);
					} else
						fprintf(stderr, "Skipping command...\n");
				}
			}
		}
		cmd += pos;
	}

	free(_cmd);
}

/* (unused)
static cmd_mm_entry_t *con_iterate_entry(int ID, int *counter) {
	byte *retval;
	con_init();

	if (*counter >= cmd_mm[ID].entries)
		return 0;
	retval = cmd_mm[ID].data;
	retval += (*counter) * cmd_mm[ID].size_per_entry;

	(*counter)++;

	return (cmd_mm_entry_t *)retval;
}*/

static cmd_mm_entry_t *con_alloc_page_entry(int ID) {
	int entry;

	con_init();

	if (cmd_mm[ID].entries >= cmd_mm[ID].allocated) {
		int nextsize = cmd_mm[ID].allocated;
		if (nextsize >= 64)
			nextsize += 16;
		else
			nextsize <<= 1;

		cmd_mm[ID].data = realloc(cmd_mm[ID].data, nextsize * cmd_mm[ID].size_per_entry);
		cmd_mm[ID].allocated = nextsize;
	}

	_lists_need_sorting |= (1 << ID);

	entry = cmd_mm[ID].entries++;
	return (cmd_mm_entry_t *)(((byte *)cmd_mm[ID].data) + entry * cmd_mm[ID].size_per_entry);
}

int con_hook_page(const char *name, const char *body) {
	cmd_page_t *page = (cmd_page_t *)con_alloc_page_entry(CMD_MM_DOC);

	page->name = name;
	page->description = body;

	return 0;
}

int con_hook_command(ConCommand command, const char *name, const char *param, const char *description) {
	cmd_command_t *cmd = NULL;
	unsigned int i;

	if (NULL == name) {
		sciprintf("console.c: con_hook_command(): NULL passed for name\n");
		return -1;
	}

	if (command == NULL)
		return 1;

	if (param == NULL)
		param = "";

	if (description == NULL)
		description = "";

	i = 0;
	while (param[i] != 0) {
		switch (param[i]) {
		case '*':
			if (param[i + 1] != 0)
				return 1;
			if (i == 0)
				return 1;
		case 'h':
		case '!':
		case 'i':
		case 'a':
		case 's':
		case 'r':
			break;
		default:
			return 1;
		}
		i++;
	}
	cmd = (cmd_command_t *)con_alloc_page_entry(CMD_MM_CMD);

	cmd->command = command;
	cmd->name = name;
	cmd->param = param;
	cmd->description = description;

	((SciEngine *)g_engine)->_console->con_hook_command(command, name, param, description);

	return 0;
}

int con_hook_int(int *pointer, const char *name, const char *description) {
	cmd_var_t *var;

	if (pointer == NULL)
		return 1;

	if (description == NULL)
		description = "";

	var = (cmd_var_t *) con_alloc_page_entry(CMD_MM_VAR);

	var->var.intp = pointer;
	var->name = name;
	var->description = description;

	return 0;
}

// Console commands and support functions

static ResourceType parseResourceType(const char *resid) {
	// Gets the resource number of a resource string, or returns -1
	ResourceType res = kResourceTypeInvalid;

	for (int i = 0; i < kResourceTypeInvalid; i++)
		if (strcmp(getResourceTypeName((ResourceType)i), resid) == 0)
			res = (ResourceType)i;

	return res;
}

static void _cmd_print_command(cmd_mm_entry_t *data, int full) {
	const char *paramseeker = ((cmd_command_t *)data)->param;

	if (full) {
		sciprintf("SYNOPSIS\n\n  %s (%s) ", data->name, paramseeker);

		while (*paramseeker) {
			switch (*paramseeker) {
			case '!':
				break;
			case 'i':
				sciprintf(" (int)");
				break;
			case 'a':
				sciprintf(" (addr)");
				break;
			case 's':
				sciprintf(" (string)");
				break;
			case 'h':
				sciprintf(" (hexbyte)");
				break;
			case '*':
				sciprintf("*");
				break;
			default:
				sciprintf(" (Unknown(%c))", *paramseeker);
			}
			paramseeker++;
		}

		sciprintf("\n\nDESCRIPTION\n\n  %s", data->description);
	} else
		sciprintf(" %s", data->name);
}

static void _cmd_print_var(cmd_mm_entry_t *data, int full) {
	cmd_var_t *var = (cmd_var_t *) data;
	if (full)
		sciprintf("VALUE\n\n");
	sciprintf("  %s = %d\n", var->name, *(var->var.intp));

	if (full)
		sciprintf("\n\nDESCRIPTION\n\n  %s", data->description);
}

static void _cmd_print_page(cmd_mm_entry_t *data, int full) {
	if (full)
		sciprintf("\n\nDESCRIPTION\n\n  %s\n", data->description);
	else
		sciprintf("%s\n", data->name);
}

static int c_list(EngineState *s, const Common::Array<cmd_param_t> &cmdParams) {
	if (_lists_need_sorting)
		con_sort_all();

	if (cmdParams.size() == 0) {
		sciprintf("usage: list [type]\nwhere type is one of the following:\n"
		          "cmds       - lists all commands\n"
		          "vars       - lists all variables\n"
		          "docs       - lists all misc. documentation\n"
		          "\n"
		          "[resource] - lists all [resource]s");
	} else if (cmdParams.size() == 1) {
		const char *mm_subsects[3] = {"cmds", "vars", "docs"};
		int mm_found = -1;
		int i;

		for (i = 0; i < 3; i++)
			if (mm_subsects[i] && !strcmp(mm_subsects[i], cmdParams[0].str))
				mm_found = i;

		if (mm_found >= 0)
			for (i = 0; i < cmd_mm[mm_found].entries; i++)
				cmd_mm[mm_found].print((cmd_mm_entry_t *)(((byte *)cmd_mm[mm_found].data) + i * cmd_mm[mm_found].size_per_entry), 0);
		else {
			if (!s) {
				sciprintf("You need a state to do that!\n");
				return 1;
			} else {
				ResourceType res = parseResourceType(cmdParams[0].str);
				if (res == kResourceTypeInvalid)
					sciprintf("Unknown resource type: '%s'\n", cmdParams[0].str);
				else {
					for (i = 0; i < sci_max_resource_nr[s->resmgr->_sciVersion]; i++)
						if (s->resmgr->testResource(res, i))
							sciprintf("%s.%03d\n", getResourceTypeName((ResourceType)res), i);
				}
			}
		}
	} else
		sciprintf("list can only be used with one argument");
	return 0;
}

static int c_set(EngineState *s, const Common::Array<cmd_param_t> &cmdParams) {
	cmd_var_t *var = (cmd_var_t *)cmd_mm_find(cmdParams[0].str, CMD_MM_VAR);

	if (var)
		*(var->var.intp) = cmdParams[1].val;

	return 0;
}

static int c_print(EngineState *s, const Common::Array<cmd_param_t> &cmdParams) {
	cmd_var_t *var = (cmd_var_t *)cmd_mm_find(cmdParams[0].str, CMD_MM_VAR);

	if (var)
		sciprintf("%d", *(var->var.intp));
	else
		sciprintf("Not defined.");

	return 0;
}

static int c_hexgrep(EngineState *s, const Common::Array<cmd_param_t> &cmdParams) {
	int i, seeklen, resnr, resmax;
	unsigned char *seekstr = NULL;
	Resource *script = NULL;
	char *dot = (char *)strchr(cmdParams[0].str, '.');
	ResourceType restype;

	if (NULL == s) {
		fprintf(stderr, "console.c: c_hexgrep(): NULL passed for s\r\n");
		return(-1);
	}

	seeklen = cmdParams.size() - 1;
	seekstr = (unsigned char *)malloc(seeklen);

	if (NULL == seekstr) {
		fprintf(stderr, "console.c: c_hexgrep(): malloc failed for seekstr\r\n");
		return(-1);
	}

	for (i = 0; i < seeklen; i++)
		seekstr[i] = (byte)cmdParams[i + 1].val;

	if (dot) {
		*dot = 0;
		resmax = resnr = atoi(dot + 1);
	} else {
		resnr = 0;
		resmax = 999;
	}

	restype = parseResourceType(cmdParams[0].str);
	if (restype == kResourceTypeInvalid) {
		sciprintf("Unknown resource type \"%s\"\n", cmdParams[0].str);
		free(seekstr);
		return 1;
	}

	for (; resnr <= resmax; resnr++)
		if ((script = s->resmgr->findResource(restype, resnr, 0))) {
			unsigned int seeker = 0, seekerold = 0;
			int comppos = 0;
			int output_script_name = 0;

			while (seeker < script->size) {
				if (script->data[seeker] == seekstr[comppos]) {
					if (comppos == 0)
						seekerold = seeker;

					comppos++;

					if (comppos == seeklen) {
						comppos = 0;
						seeker = seekerold + 1;

						if (!output_script_name) {
							sciprintf("\nIn %s.%03d:\n", getResourceTypeName((ResourceType)restype), resnr);
							output_script_name = 1;
						}
						sciprintf("   0x%04x\n", seekerold);
					}
				} else
					comppos = 0;

				seeker++;
			}
		}

	free(seekstr);

	return 0;
}

} // End of namespace Sci
