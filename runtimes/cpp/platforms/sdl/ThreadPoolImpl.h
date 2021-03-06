/* Copyright 2013 David Axmark

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef THREADPOOLIMPL_H
#define THREADPOOLIMPL_H

#include <SDL/SDL.h>

class MoSyncThread {
public:
	void start(int (SDLCALL * func)(void*), void* arg);
	int join();
	static void sleep(unsigned int ms);
	bool isCurrent();	//returns true if this thread is the current thread.
private:
	SDL_Thread* mThread;
};

class MoSyncSemaphore {
public:
	MoSyncSemaphore();
	~MoSyncSemaphore();
	void wait();
	void post();
private:
	SDL_sem* mSem;
};

#endif	//THREADPOOLIMPL_H
