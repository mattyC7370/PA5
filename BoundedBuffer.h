#ifndef BoundedBuffer_h
#define BoundedBuffer_h

#include <stdio.h>
#include <queue>
#include <string>
#include <pthread.h>
#include <thread>
#include <mutex>
#include <assert.h>
#include <condition_variable>

using namespace std;

class BoundedBuffer
{
private:
	int cap; // max number of items in the buffer
	queue<vector<char>> q;	/* the queue of items in the buffer. Note   
	that each item a sequence of characters that is best represented by a vector<char> for 2 reasons:
	1. An STL std::string cannot keep binary/non-printables
	2. The other alternative is keeping a char* for the sequence and an integer length (i.e., the items can be of variable length).
	While this would work, it is clearly more tedious */

	// add necessary synchronization variables and data structures 

	//for thread-safety
	mutex m;
	condition_variable data_avalible;	//for wait by pop function, signaled by push functions
	condition_variable slot_avalible;	//waited on by the push, signaled by pop
	
public:
	BoundedBuffer(int _cap){
		cap = _cap;
	}
	~BoundedBuffer(){

	}

	void push(char* data, int len){
		
		//0. Convert the incoming byte sequence given by data and len into a vector<char>
		vector<char> d (data, data + len);
		
		//1. Wait until there is room in the queue (i.e., queue lengh is less than cap)
		unique_lock<mutex> l(m);
		slot_avalible.wait(l, [this]{return q.size() < cap;});
		
		
		
		//2. Then push the vector at the end of the queue, watch out for race condition
		
		q.push(d);
		l.unlock();
		//3. Wake up the threads that are waiting 		wake up pop() threads
		data_avalible.notify_one();
		
		//TBD
		
		
	}

	
	//pop is like cread
	int pop(char* buf, int bufcap){
		//suspend thread that tried to pop until there is something in the queue 
		//1. Wait until the queue has at least 1 item
		unique_lock<mutex> l(m);
		data_avalible.wait(l, [this]{return q.size() > 0;});
		
		
		//2. pop the front item of the queue. The popped item is a vector<char>
		
		vector<char> d = q.front();
		q.pop();
		l.unlock();
		
		//3. Convert the popped vector<char> into a char*, copy that into buf, make sure that vector<char>'s length is <= bufcap
		assert(d.size() <= bufcap);
		memcpy(buf, d.data(), d.size());
		
		
		
		
		
		//4. wake up any potentially sleeping push() function
		slot_avalible.notify_one();
		
		
		//5. Return the vector's length to the caller so that he knows many bytes were popped
		return d.size();
		
		
	}
};

#endif /* BoundedBuffer_ */