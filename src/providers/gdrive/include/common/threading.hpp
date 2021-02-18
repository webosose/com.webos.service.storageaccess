#ifndef __common_THREADING_HPP__
#define __common_THREADING_HPP__

#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include "common/misc.hpp"

namespace common {

class Lock {
    CLASS_NOCOPY(Lock)
    public:
        Lock(){}
        virtual void lock() = 0;
        virtual void unlock() = 0;
        virtual ~Lock() {}
};

class Mutex : public Lock {
    CLASS_NOCOPY(Mutex)
    public:
        Mutex() {
            pthread_mutex_init(&_lock, NULL);
        }
        ~Mutex() {
            pthread_mutex_destroy(&_lock);
        }
        void lock() {
            pthread_mutex_lock(&_lock);
        }
        void unlock() {
            pthread_mutex_unlock(&_lock);
        }
    private:
        pthread_mutex_t _lock;
        friend class Condition;
};

class RecursiveMutex : public Lock {
    CLASS_NOCOPY(RecursiveMutex)
    public:
        RecursiveMutex() {
            pthread_mutexattr_init(&_attr);
            pthread_mutexattr_settype(&_attr, PTHREAD_MUTEX_RECURSIVE);

            pthread_mutex_init(&_lock, &_attr);
        }
        ~RecursiveMutex() {
            pthread_mutexattr_destroy(&_attr);
            pthread_mutex_destroy(&_lock);
        }
        void lock() {
            pthread_mutex_lock(&_lock);
        }
        void unlock() {
            pthread_mutex_unlock(&_lock);
        }
    private:
        pthread_mutex_t _lock;
        pthread_mutexattr_t _attr;
        friend class Condition;
};

class ScopeLock {
    public:
        ScopeLock(Lock* lock) {
            _lock = lock;
            _lock->lock();
        }
        ~ScopeLock() {
            _lock->unlock();
        }
    private:
        Lock* _lock;
};

class Condition {
    CLASS_NOCOPY(Condition)
    public:
        Condition(Mutex* lock) {
            _lock = lock;
            pthread_cond_init(&_cond, NULL);
        }
        ~Condition(){
            pthread_cond_destroy(&_cond);
        }

        void wait() {
            pthread_cond_wait(&_cond, &_lock->_lock); 
        }

        void notify() {
            pthread_cond_signal(&_cond);
        }

        void notify_all() {
            pthread_cond_broadcast(&_cond);
        }
    private:
        pthread_cond_t _cond;
        Mutex* _lock;
};

class Runable {
    public:
        virtual void run() = 0;
        virtual ~Runable(){}
};

class Thread: public Runable {
    public:
        Thread(const Runable& runable) {
            _joinable = true;
            _active = false;
            _context = &runable;
        }
        Thread() {
            _joinable = true;
            _active = false;
            _context = NULL;
        }

        virtual void run() {
        }

        bool start() {
            if (_active == true) {
                return false;
            }
            if (_context == NULL) {
                _context = this;
            }

            int r = pthread_create(&_thread_id, NULL, &Thread::_run_thread, (void*)_context);
            if (r != 0) {
                return false;
            }
            _active = true;
            return true;
        }

        long thread_id() const { return (long)_thread_id; }

        bool join() {
            if (_joinable == false) return false;

            int r = pthread_join(_thread_id, NULL);

            if (r != 0) {
                return false;
            }
            _active = false;
            return true;
        }

        bool is_active() {
            return _active;
        }

        virtual ~Thread() {
            if (_active)
                join();
        }

    private:
        static void* _run_thread(void* context) {
            Runable * self = (Runable*)context;
            self->run();
            pthread_exit(NULL);
            return NULL;
        }

        pthread_t _thread_id;
        const Runable * _context;

        bool _active;
        bool _joinable;
};

class AsyncMethod {
    public:
        AsyncMethod(): _runable(NULL), _thread(NULL) {
        }
        void start_async(void (*fn) (void) ) {
            _check();
            typedef void (*FUNC) (void);
            class VVRunable : public Runable {
                public:
                    VVRunable(FUNC fn) : _fn(fn) {
                    }
                    void run() {
                        _fn();
                    }
                private:
                    FUNC _fn;
            };
            _runable = new VVRunable(fn);
            _thread = new Thread(*_runable);
            _thread->start();
        }

        template<class R>
        void start_async(R (*fn) (void), R* r) {
            _check();
            typedef R (*FUNC) (void);

            class RVRunable : public Runable {
                public:
                    RVRunable(FUNC fn, R* r) : _fn(fn), _r(r){
                    }
                    void run() {
                        *_r = _fn();
                    }
                private:
                    FUNC _fn;
                    R* _r;
            };

            _runable = new  RVRunable(fn, r);
            _thread = new Thread(*_runable);
            _thread->start();
        }

        template<class A1>
        void start_async(void (*fn) (A1) , A1 a1) {
            _check();
            typedef void (*FUNC) (A1);

            class VA1Runable : public Runable {
                public:
                    VA1Runable(FUNC fn, A1 a1) : _fn(fn), _a1(a1){
                    }
                    void run() {
                        _fn(_a1);
                    }
                private:
                    FUNC _fn;
                    A1 _a1;
            };

            _runable = new  VA1Runable(fn, a1);
            _thread = new Thread(*_runable);
            _thread->start();
        }

        template<class C>
        void start_async(void (C::*fn)(), C* c) {
            _check();
            typedef void (C::*FUNC)();
            class VCVRunable : public Runable {
                public:
                    VCVRunable(FUNC fn, C* c) : _fn(fn), _c(c){
                    }
                    void run() {
                        (_c->*_fn)();
                    }
                private:
                    FUNC _fn;
                    C* _c;
            };

            _runable = new  VCVRunable(fn, c);
            _thread = new Thread(*_runable);
            _thread->start();
        }

        template<class A1, class R>
        void start_async(R (*fn) (A1), A1 a1, R*r) {
            _check();
            typedef R (*FUNC) (A1);

            class RA1Runable : public Runable {
                public:
                    RA1Runable(FUNC fn, A1 a1, R* r) : _fn(fn), _a1(a1), _r(r) {
                    }

                    void run() {
                        *_r = _fn(_a1);
                    }

                private:
                    FUNC _fn;
                    A1 _a1;
                    R* _r;
            };

            _runable = new RA1Runable(fn, a1, r);
            _thread = new Thread(*_runable);
            _thread->start();
        }

        template<class C, class R>
        void start_async(R (C::*fn) (), C* c, R* r) {
            _check();
            typedef R (C::*FUNC) ();

            class RCVRunable : public Runable {
                public:
                    RCVRunable(FUNC fn, C* c, R* r) : _fn(fn), _c(c), _r(r) {
                    }

                    void run() {
                        *_r = (_c->*_fn)();
                    }

                private:
                    FUNC _fn;
                    C* _c;
                    R* _r;
            };

            _runable = new RCVRunable(fn, c, r);
            _thread = new Thread(*_runable);
            _thread->start();
        }


        template<class A1, class A2>
        void start_async(void (*fn) (A1, A2) , A1 a1, A2 a2) {
            _check();
            typedef void (*FUNC) (A1, A2);

            class VA2Runable : public Runable {
                public:
                    VA2Runable(FUNC fn, A1 a1, A2 a2) : _fn(fn), _a1(a1), _a2(a2) {
                    }
                    void run() {
                        _fn(_a1, _a2);
                    }
                private:
                    FUNC _fn;
                    A1 _a1;
                    A2 _a2;
            };

            _runable = new  VA2Runable(fn, a1, a2);
            _thread = new Thread(*_runable);
            _thread->start();
        }

        template<class C, class A1>
        void start_async(void (C::*fn) (A1), C* c, A1 a1) {
            _check();
            typedef void (C::*FUNC) (A1);

            class VCA1Runable : public Runable {
                public:
                    VCA1Runable(FUNC fn, C* c, A1 a1) : _fn(fn), _c(c), _a1(a1) {
                    }
                    void run() {
                        (_c->*_fn)(_a1);
                    }
                private:
                    FUNC _fn;
                    C* _c;
                    A1 _a1;
            };

            _runable = new  VCA1Runable(fn, c, a1);
            _thread = new Thread(*_runable);
            _thread->start();
        }


        template<class A1, class A2, class R>
        void start_async(R (*fn) (A1, A2), A1 a1, A2 a2, R*r) {
            _check();
            typedef R (*FUNC) (A1, A2);

            class RA2Runable : public Runable {
                public:
                    RA2Runable(FUNC fn, A1 a1, A2 a2, R* r) : _fn(fn), _a1(a1), _a2(a2), _r(r) {
                    }

                    void run() {
                        *_r = _fn(_a1, _a2);
                    }

                private:
                    FUNC _fn;
                    A1 _a1;
                    A2 _a2;
                    R* _r;
            };

            _runable = new RA2Runable(fn, a1, a2, r);
            _thread = new Thread(*_runable);
            _thread->start();
        }

        template<class C, class A1, class R>
        void start_async(R (C::*fn) (A1), C* c, A1 a1, R*r) {
            _check();
            typedef R (C::*FUNC) (A1);

            class RCA1Runable : public Runable {
                public:
                    RCA1Runable(FUNC fn, C* c, A1 a1, R* r) : _fn(fn), _c(c), _a1(a1), _r(r) {
                    }

                    void run() {
                        *_r = (_c->*_fn)(_a1);
                    }

                private:
                    FUNC _fn;
                    C* _c;
                    A1 _a1;
                    R* _r;
            };

            _runable = new RCA1Runable(fn, c, a1, r);
            _thread = new Thread(*_runable);
            _thread->start();
        }


        template<class A1, class A2, class A3>
        void start_async(void (*fn) (A1, A2, A3) , A1 a1, A2 a2, A3 a3) {
            _check();
            typedef void (*FUNC) (A1, A2, A3);

            class VA3Runable : public Runable {
                public:
                    VA3Runable(FUNC fn, A1 a1, A2 a2, A3 a3) : _fn(fn), _a1(a1), _a2(a2), _a3(a3){
                    }
                    void run() {
                        _fn(_a1, _a2, _a3);
                    }
                private:
                    FUNC _fn;
                    A1 _a1;
                    A2 _a2;
                    A3 _a3;
            };

            _runable = new  VA3Runable(fn, a1, a2, a3);
            _thread = new Thread(*_runable);
            _thread->start();
        }

        template<class C, class A1, class A2>
        void start_async(void (C::*fn) (A1, A2) , C* c, A1 a1, A2 a2) {
            _check();
            typedef void (C::*FUNC) (A1, A2);

            class VCA2Runable : public Runable {
                public:
                    VCA2Runable(FUNC fn, C* c, A1 a1, A2 a2) : _fn(fn), _c(c), _a1(a1), _a2(a2){
                    }
                    void run() {
                        (_c->*_fn)(_a1, _a2);
                    }
                private:
                    FUNC _fn;
                    C* _c;
                    A1 _a1;
                    A2 _a2;
            };

            _runable = new  VCA2Runable(fn, c, a1, a2);
            _thread = new Thread(*_runable);
            _thread->start();
        }

        template<class A1, class A2, class A3, class R>
        void start_async(R (*fn) (A1, A2, A3), A1 a1, A2 a2, A3 a3, R*r) {
            _check();
            typedef R (*FUNC) (A1, A2, A3);

            class RA3Runable : public Runable {
                public:
                    RA3Runable(FUNC fn, A1 a1, A2 a2, A3 a3, R* r) : _fn(fn), _a1(a1), _a2(a2), _a3(a3), _r(r) {
                    }

                    void run() {
                        *_r = _fn(_a1, _a2, _a3);
                    }

                private:
                    FUNC _fn;
                    A1 _a1;
                    A2 _a2;
                    A3 _a3;
                    R* _r;
            };

            _runable = new RA3Runable(fn, a1, a2, a3, r);
            _thread = new Thread(*_runable);
            _thread->start();
        }

        template<class C, class A1, class A2, class R>
        void start_async(R (C::*fn) (A1, A2), C* c, A1 a1, A2 a2, R*r) {
            _check();
            typedef R (C::*FUNC) (A1, A2);

            class RCA2Runable : public Runable {
                public:
                    RCA2Runable(FUNC fn, C* c, A1 a1, A2 a2, R* r) : _fn(fn), _c(c), _a1(a1), _a2(a2), _r(r) {
                    }

                    void run() {
                        *_r = (_c->*_fn)(_a1, _a2);
                    }

                private:
                    FUNC _fn;
                    C* _c;
                    A1 _a1;
                    A2 _a2;
                    R* _r;
            };

            _runable = new RCA2Runable(fn, c, a1, a2, r);
            _thread = new Thread(*_runable);
            _thread->start();
        }


        void wait() {
          _thread->join();
        }

        ~AsyncMethod() {
            _thread->join();
            if (_runable) delete _runable;
            if (_thread) delete _thread;
        }

    private:
        Runable *_runable;
        Thread *_thread;


        void _check(){
          if (_runable != nullptr) {
            _thread->join();
            delete _runable;
            delete _thread;
          }
        }
};

}

#endif
