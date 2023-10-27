/*
===============================================================================
Copyright (C) 2020 Rachael Alexanderson

Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
 may be used to endorse or promote products derived from this software without
 specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.
===============================================================================
*/

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <algorithm>
#include <cctype>

#include "config-converter.h"
#include "version.h"

int main(int argc, char** argv)
{
	ifstream gzdoomini;
	ofstream keyconf, defcvars, defbinds;
	string line, streamcommand;
	string game, mod;
	string aliasname;

	char usernameconfig[257];

	size_t found;
	uint8_t handletype = 0;

	cout << "GZDoom Config Converter " << VERSION << endl;
	cout << "Copyright (c) " << COPYRIGHT_YEAR << " Rachael Alexanderson" << endl;
	cout << "This software is released under the terms of the BSD 3-clause license" << endl;
	cout << "Please review the source code for further information" << endl;
	cout << "Github repo: https://github.com/madame-rachelle/gzdoom-config-converter" << endl << endl;

	if (argc <= 1)
	{
		cout << "Must specify config name (ie Doom/Hexen/etc)!\n";
		return 0;
	}

	game.assign(argv[1]);
	if (argc >= 3)
		mod.assign(argv[2]);
	else
		mod.assign("");

	sections section_list[] =
	{
		{ HT_ALIAS,		"alias ",		"[%s.ConsoleAliases]" 		},
		{ HT_VERSION,	"",				"[LastRun]" 				},
		{ HT_CVAR,		"set ",			"[GlobalSettings]" 			},
		{ HT_CVAR,		"set ",			"[%s.Player]" 				},
		{ HT_CVAR,		"set ",			"[%s.LocalServerInfo]" 		},
		{ HT_CVAR,		"set ",			"[%s.NetServerInfo]" 		},
		{ HT_CVAR,		"set ",			"[%s.ConsoleVariables]" 	},
		{ HT_BIND,		"bind ",		"[%s.Bindings]" 			},
		{ HT_BIND,		"doublebind ",	"[%s.DoubleBindings]" 		},
		{ HT_BIND,		"mapbind ",		"[%s.AutomapBindings]" 		},
		{ HT_BIND,		"bind ",		"[%s.%s.Bindings]" 			},
		{ HT_BIND,		"doublebind ",	"[%s.%s.DoubleBindings]" 	},
		{ HT_BIND,		"mapbind ",		"[%s.%s.AutomapBindings]" 	},
	};

	string blacklisted_cvars[] =
	{
		"disablecrashlog",
		"gl_control_tear",
		"in_mouse",
		"joy_dinput",
		"joy_ps2raw",
		"joy_xinput",
		"k_allowfullscreentoggle",
		"k_mergekeys",
		"m_swapbuttons",
		"queryiwad_key",
		"vid_gpuswitch",
		"vr_enable_quadbuffered",
		"m_filter",
		"gl_debug",
		"vid_adapter",
		"sys_statsenabled47",
		"sys_statsenabled49",
		"save_dir",
		"sys_statsport",
		"sys_statshost",
		"sentstats_hwr_done",
	};

	// check for the presence of output files, and error out if they're found
	// this is to prevent loss of data
	if (fexists("keyconf.txt") || fexists("defcvars.txt") || fexists("defbinds.txt"))
	{
		cout << "Current folder may not have any of the following files:" << endl;
		cout << "   keyconf.txt, defcvars.txt, or defbinds.txt" << endl << endl;
		cout << "To prevent loss of data, please move these files out of the current folder or delete them." << endl;
		return 0;
	}

	keyconf.open("keyconf.txt");
	defcvars.open("defcvars.txt");
	defbinds.open("defbinds.txt");

#ifdef _WIN32
	snprintf(usernameconfig, 256, "gzdoom-%s.ini", getenv("USERNAME"));
#endif

	// check for the presence of gzdoom.ini and open it
	if (fexists("gzdoom_portable.ini"))
		gzdoomini.open("gzdoom_portable.ini");
#ifdef _WIN32
	else if (fexists(usernameconfig))
		gzdoomini.open(usernameconfig);
#endif
	else if (fexists("gzdoom.ini"))
		gzdoomini.open("gzdoom.ini");
	else
	{
		cout << "gzdoom.ini or gzdoom_portable.ini does not exist!\n";
		return 0;
	}

	while ( getline(gzdoomini, line) )
	{
		if (line.empty())
			continue;
		if (line.front() == '[' && line.back() == ']')
		{
			handletype = HT_NONE;
			// section
			for (const sections &s : section_list)
			{
				char checkheader[257], checkline[257];
				snprintf(checkheader, 256, s.header.c_str(), game.c_str(), mod.c_str());
				for (uint16_t j = 0; j < 256; j++)
				{
					if (checkheader[j] >= 'A' && checkheader[j] <= 'Z')
						checkheader[j] += 32;
					checkline[j] = line[j];
					if (checkline[j] >= 'A' && checkline[j] <= 'Z')
						checkline[j] += 32;
				}
				checkheader[256] = checkline[256] = 0;
				if (!strcmp(checkheader, checkline))
				{
					handletype = s.handlingtype;
					streamcommand = s.command;
				}
			}
		}
		else if (line.front() == '#')
			{ } //comment, ignore
		else if ((found = line.find("=")) != string::npos && handletype != HT_NONE)
		{
			string var = line.substr(0, found);
			string value = line.substr(found + 1);
			const char quote[2] = {34, 0};

			switch (handletype)
			{
			case HT_CVAR:
				for (const string &bl : blacklisted_cvars)
				{
					std::transform(var.begin(), var.end(), var.begin(),
						[](unsigned char c){ return std::tolower(c); });

					// ignore blacklisted cvars
					if (!bl.compare(var))
						break;
				}
				defcvars << streamcommand << var << " " << quote << value << quote << endl;
				break;
			case HT_ALIAS:
				if (!var.compare("Name"))
					aliasname = value;
				if (!var.compare("Command") || !var.compare("Value"))
					keyconf << streamcommand << aliasname << " " << quote << value << quote << endl;
				break;
			case HT_VERSION:
				defcvars << "version " << value << endl;
				break;
			case HT_BIND:
				defbinds << streamcommand << var << " " << quote << value << quote << endl;
				break;
			default:
				break;
			}
		}
	}

	defbinds.close();
	defcvars.close();
	keyconf.close();
	gzdoomini.close();
	return 0;
}

