#define SAVE
/*
    This file is part of the CL2 script language interpreter.

    Gunnar Selke <gunnar@gmx.info>

    CL2 is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    CL2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CL2; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "cl2.h"

#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

int main(char argc, char **args)
{
	srand(time(0));

	try
	{
		CLContext context;
		CLMathModule math;

		context.addModule(&math);

#ifdef SAVE
		const char *file = argc > 1 ? args[1] : "hello.cl2";
		cout << "Starte " << file << endl;
		CLValue mainfunc = CLCompiler::compile(file);

		CLValue thr(new CLThread());
		GET_THREAD(thr)->init(mainfunc);

		for (int i=0; i<1000; ++i)
		{
			context.roundRobin();
			context.unmarkObjects();
			context.markObjects();
			context.sweepObjects();
			context.freeFinalized();
		}

		ofstream outputfile("dump.bin");
		CLSerialSaver ss(outputfile);
		context.save(ss);
#endif
#ifndef SAVE
		ifstream inputfile("dump.bin");
		CLSerialLoader sl(inputfile);
		context.load(sl);

		while (context.countRunningThreads() > 0)
		{
			context.roundRobin();
			context.unmarkObjects();
			context.markObjects();
			context.sweepObjects();
			context.freeFinalized();
		}
#endif

		context.clear();
#ifdef DEBUG
		//cout << "Instructions executed: " << icount << endl;
#endif

	} catch (CLParserException err) {
		cout << err.what() << endl;
	} catch (std::runtime_error err) {
		cout << err.what() << endl;
	}
	cout << endl;
}

