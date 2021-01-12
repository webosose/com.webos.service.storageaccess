#ifndef __common_THREADPOOL_HPP__
#define __common_THREADPOOL_HPP__

#include "common/misc.hpp"
#include "common/queue.hpp"
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <functional>

namespace common {

typedef void (*func) (void*);

class Job {
    public:
        virtual void run() = 0;
        virtual ~Job() {}
};

#define TP_IMMEDIATE_SHUTDOWN 0
#define TP_POSTPONED_SHUTDOWN 1

class ThreadPool {
    CLASS_NOCOPY(ThreadPool)
    public:
        ThreadPool(size_t pool_size = get_ncpu(), size_t queue_size = 2 * get_ncpu())
            :_task_queue(queue_size), _pool_size(pool_size)
        {
            pthread_mutex_init(&_lock, NULL);
            _stop = false;
            _shutdown = -1;
            assert(_pool_size > 0);
            _pool = (pthread_t*) malloc(sizeof(pthread_t) * _pool_size);
            assert(_pool != NULL);
            _active_count = 0;

            pthread_mutex_lock(&_lock);
            size_t i;
            for(i = 0; i < _pool_size; i ++ ) {
                int ret = pthread_create(&_pool[i], NULL, _run_thread, (void*)this);
                if (ret != 0) {
                    pthread_mutex_unlock(&_lock);
                    destroy();
                    break;
                } else {
                    _active_count ++;
                }
            }
            if (i == _pool_size) pthread_mutex_unlock(&_lock);
        }

        void destroy(int shutdown = TP_IMMEDIATE_SHUTDOWN) {
            if (shutdown != TP_IMMEDIATE_SHUTDOWN && shutdown != TP_POSTPONED_SHUTDOWN) {
                _shutdown = TP_IMMEDIATE_SHUTDOWN;
            } else {
                _shutdown = shutdown;
            }
            for(size_t i = 0; i < _active_count; i ++ ) {
                pthread_join(_pool[i], NULL);
            }
            _task_queue.clear();
            free(_pool);
            _pool = NULL;
            _stop = true;
        }

        bool add(void (*fn) (void) ) {
            typedef void (*FUNC) (void);
            class VVJob : public Job {
                public:
                    VVJob(FUNC fn) : _fn(fn) {
                    }
                    void run() {
                        _fn();
                    }
                private:
                    FUNC _fn;
            };
            _task_queue.push(new VVJob(fn));
            return true;
        }

        template<class C>
        bool add(void (C::*fn) (void), C* c) {
            typedef void (C::*FUNC) (void);
            class VCVJob : public Job {
                public:
                    VCVJob(FUNC fn, C* c) : _fn(fn), _c(c) {
                    }
                    void run() {
                        (_c->*_fn)();
                    }
                private:
                    FUNC _fn;
                    C* _c;
            };
            _task_queue.push(new VCVJob(fn, c));
            return true;
        }


        template<class R>
        bool add(R (*fn)(void), R* r) {
            typedef R (*FUNC) (void);

            class RVJob : public Job {
                public:
                    RVJob(FUNC fn, R* r) : _fn(fn), _r(r){
                    }
                    void run() {
                        *_r = _fn();
                    }
                private:
                    FUNC _fn;
                    R* _r;
            };

            _task_queue.push(new  RVJob(fn, r));
            return true;
        }

        template<class C, class R>
        bool add(R (C::*fn)(void), C* c, R* r) {
            typedef R (C::*FUNC) (void);

            class RCVJob : public Job {
                public:
                    RCVJob(FUNC fn, C* c, R* r) : _fn(fn), _c(c), _r(r){
                    }
                    void run() {
                        *_r = (_c->*_fn)();
                    }
                private:
                    FUNC _fn;
                    C* _c;
                    R* _r;
            };

            _task_queue.push(new  RCVJob(fn, c, r));
            return true;
        }


        template<class A1>
        bool add(void (*fn) (A1) , A1 a1) {
            typedef void (*FUNC) (A1);

            class VA1Job : public Job {
                public:
                    VA1Job(FUNC fn, A1 a1) : _fn(fn), _a1(a1){
                    }
                    void run() {
                        _fn(_a1);
                    }
                private:
                    FUNC _fn;
                    A1 _a1;
            };

            _task_queue.push(new  VA1Job(fn, a1));
            return true;
        }

        template<class C, class A1>
        bool add(void (C::*fn) (A1) , C* c, A1 a1) {
            typedef void (C::*FUNC) (A1);

            class VCA1Job : public Job {
                public:
                    VCA1Job(FUNC fn, C* c, A1 a1) : _fn(fn), _c(c), _a1(a1){
                    }
                    void run() {
                        (_c->*_fn)(_a1);
                    }
                private:
                    FUNC _fn;
                    C* _c;
                    A1 _a1;
            };

            _task_queue.push(new  VCA1Job(fn, c, a1));
            return true;
        }

        template<class A1, class R>
        bool add(R (*fn) (A1), A1 a1, R*r) {
            typedef R (*FUNC) (A1);

            class RA1Job : public Job {
                public:
                    RA1Job(FUNC fn, A1 a1, R* r) : _fn(fn), _a1(a1), _r(r) {
                    }

                    void run() {
                        *_r = _fn(_a1);
                    }

                private:
                    FUNC _fn;
                    A1 _a1;
                    R* _r;
            };

            _task_queue.push(new RA1Job(fn, a1, r));
            return true;
        }

        template<class C, class A1, class R>
        bool add(R (C::*fn) (A1), C* c, A1 a1, R*r) {
            typedef R (C::*FUNC) (A1);

            class RCA1Job : public Job {
                public:
                    RCA1Job(FUNC fn, C* c, A1 a1, R* r) : _fn(fn), _c(c), _a1(a1), _r(r) {
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

            _task_queue.push(new RCA1Job(fn, c, a1, r));
            return true;
        }

        template<class A1, class A2>
        bool add(void (*fn) (A1, A2) , A1 a1, A2 a2) {
            typedef void (*FUNC) (A1, A2);

            class VA2Job : public Job {
                public:
                    VA2Job(FUNC fn, A1 a1, A2 a2) : _fn(fn), _a1(a1), _a2(a2) {
                    }
                    void run() {
                        _fn(_a1, _a2);
                    }
                private:
                    FUNC _fn;
                    A1 _a1;
                    A2 _a2;
            };

            _task_queue.push(new  VA2Job(fn, a1, a2));
            return true;
        }

        template<class C, class A1, class A2>
        bool add(void (C::*fn) (A1, A2) , C* c, A1 a1, A2 a2) {
            typedef void (C::*FUNC) (A1, A2);

            class VCA2Job : public Job {
                public:
                    VCA2Job(FUNC fn, C* c, A1 a1, A2 a2) : _fn(fn), _c(c), _a1(a1), _a2(a2) {
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

            _task_queue.push(new  VCA2Job(fn, c, a1, a2));
            return true;
        }

        template<class A1, class A2, class R>
        bool add(R (*fn) (A1, A2), A1 a1, A2 a2, R*r) {
            typedef R (*FUNC) (A1, A2);

            class RA2Job : public Job {
                public:
                    RA2Job(FUNC fn, A1 a1, A2 a2, R* r) : _fn(fn), _a1(a1), _a2(a2), _r(r) {
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

            _task_queue.push(new RA2Job(fn, a1, a2, r));
            return true;
        }

        template<class C, class A1, class A2, class R>
        bool add(R (C::*fn) (A1, A2), C* c, A1 a1, A2 a2, R*r) {
            typedef R (C::*FUNC) (A1, A2);

            class RCA2Job : public Job {
                public:
                    RCA2Job(FUNC fn, C* c, A1 a1, A2 a2, R* r) : _fn(fn), _c(c), _a1(a1), _a2(a2), _r(r) {
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

            _task_queue.push(new RCA2Job(fn, c, a1, a2, r));
            return true;
        }


        template<class A1, class A2, class A3>
        bool add(void (*fn) (A1, A2, A3) , A1 a1, A2 a2, A3 a3) {
            typedef void (*FUNC) (A1, A2, A3);

            class VA3Job : public Job {
                public:
                    VA3Job(FUNC fn, A1 a1, A2 a2, A3 a3) : _fn(fn), _a1(a1), _a2(a2), _a3(a3){
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

            _task_queue.push(new  VA3Job(fn, a1, a2, a3));
            return true;
        }

        template<class A1, class A2, class A3, class R>
        bool add(R (*fn) (A1, A2, A3), A1 a1, A2 a2, A3 a3, R*r) {
            typedef R (*FUNC) (A1, A2, A3);

            class RA3Job : public Job {
                public:
                    RA3Job(FUNC fn, A1 a1, A2 a2, A3 a3, R* r) : _fn(fn), _a1(a1), _a2(a2), _a3(a3), _r(r) {
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

            _task_queue.push(new RA3Job(fn, a1, a2, a3, r));
            return true;
        }
        

        ~ThreadPool() {
            if (!_stop) {
                destroy();
            }
        }
    private:
        Queue<Job*> _task_queue;
        pthread_mutex_t _lock;
        pthread_t* _pool;
        int _shutdown;
        size_t _pool_size;
        bool _stop;
        size_t _active_count;

        static void* _run_thread(void* context) {
            ThreadPool* self = (ThreadPool*)context;

            for(;;) {
                if (self->_shutdown == TP_IMMEDIATE_SHUTDOWN) {
                    break;
                } else if (self->_shutdown == TP_POSTPONED_SHUTDOWN && self->_task_queue.size() == 0) {
                    //fprintf(stderr, "Postponed shutdown");
                    break;
                }

                Job* job = nullptr;
                if (self->_task_queue.try_pop(&job)) {
                    job->run();
                    delete job;
                } else {
                    struct timespec ts;
                    ts.tv_sec = 0;
                    ts.tv_nsec = 1000; // 1us
                    nanosleep(&ts, NULL);
                }
            }
            pthread_exit(NULL);
            return NULL;
        }
};

}

#endif
