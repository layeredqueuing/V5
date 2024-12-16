/*  -*- c++ -*-
 *
 * Message passing data structure.
 *
 * ------------------------------------------------------------------------
 * $Id: rendezvous.h 17495 2024-11-21 21:38:47Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _LQSIM_RENDEZVOUS_H
#define _LQSIM_RENDEZVOUS_H

#include <condition_variable>
#include <deque>
#include <mutex>
#include <utility>

namespace Instance {
    class Instance;
}

namespace Rendezvous {
    template <typename in_t, typename out_t>
    class Rendezvous {				/* send rendezvous struct		*/
	friend class Instance::Instance;

    private:
	Rendezvous( const Rendezvous& ) = delete;
	Rendezvous& operator=( const Rendezvous& ) = delete;

    public:
	Rendezvous() : _mutex(), _send_cv(), _recv_cv() {}
	virtual ~Rendezvous() {}

	void send( const in_t in, out_t* out ) {
	    std::unique_lock<std::mutex> lock(_mutex);
	    /* arguments are pointers to in and out messages. */
	    while ( is_full() ) { // (i.e. _send != nullptr),
		_send_cv.wait(lock);
	    }
	    put( in, out );
#if 0
	    messsage.first.reply_cv.wait(lock);
#endif
	}
    
	void receive( in_t* in ) {
	    std::unique_lock<std::mutex> lock(_mutex);
	    /* return the pointer to the send buffer */
	    while ( is_empty() ) { // (i.e. _send == nullptr ) then 
		_recv_cv.wait();
	    }
	    get( in );
	}
    
	void reply( const out_t out ) {
	    pop( out );
	    _recv_cv.notify_one();
	}

    protected:
	/* All these methods needed to be executed INSIDE the critical section */
	virtual bool is_empty() const = 0;
	virtual bool is_full() const = 0;
	virtual void put( const in_t in, out_t* out ) = 0;	/* send */
	virtual void get( in_t* in ) = 0;			/* receive */
	virtual void pop( const out_t out ) = 0;		/* reply */

    private:
	std::mutex _mutex;
	std::condition_variable _send_cv;
	std::condition_variable _recv_cv;
    };

    template <typename in_t, typename out_t>
    class Single_Server : public Rendezvous<in_t,out_t> {
    public:
	Single_Server() : Rendezvous<in_t,out_t>(), _empty(true), _messages(std::pair<in_t,out_t*>()) {}
	virtual ~Single_Server() {}

    protected:
	virtual bool is_empty() const { return _empty; }
	virtual bool is_full() const { return !_empty; }
	virtual void put( const in_t in, out_t* out ) {
	    _messages.first = in;
	    _messages.second = out;
	    _empty = false;
	}
	virtual void get( in_t* in ) {
	    *in = _messages.first;
	}
	virtual void pop( const out_t out ) {
	    *_messages.second = out;
	    _empty = true;
	}

    private:
	bool _empty;
	std::pair<in_t,out_t*> _messages;
    };

    template <typename in_t, typename out_t>
    class Multi_Server : public Rendezvous<in_t,out_t> {
    public:
	Multi_Server( size_t capacity ) : Rendezvous<in_t,out_t>(), _capacity(capacity), _messages() {}
	virtual ~Multi_Server() {}

    protected:
	virtual bool is_empty() const { return _messages.empty(); }
	virtual bool is_full() const { return _messages.size() == _capacity; }
	virtual void put( const in_t in, out_t* out ) {
	    _messages.emplace_back(std::pair<in_t,out_t*>(in,out));
	}
	virtual void get( in_t* in ) {
	    *in = _messages.front().first;
	}
	virtual void pop( const out_t out ) {
	    *_messages.front().second = out;
	    _messages.pop_front();
	}

    private:
	const size_t _capacity;
	std::deque<std::pair<in_t,out_t*>> _messages;
    };
}
#endif
