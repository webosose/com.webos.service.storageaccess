#ifndef __common_QUEUE_HPP__
#define __common_QUEUE_HPP__

#include "common/misc.hpp"
#include "common/threading.hpp"
#include <pthread.h>
#include <deque>
#include <stdio.h>

namespace common {

template<class T>
class Queue {
    public:
        Queue(size_t size = 0)
            :_lock(), _empty_cond(&_lock), _full_cond(&_lock), _size(size)
        {
        }
        ~Queue() {
            _queue.clear();
        }

        void push(const T& item) {
            _lock.lock();
            size_t queue_size = _queue.size();
            if (_size != 0 && queue_size == _size) {
                _full_cond.wait();
                _queue.push_back(item);
            } else {
                _queue.push_back(item);
                _empty_cond.notify();
            }
            _lock.unlock();
        }

        bool try_pop(T* item) {
            _lock.lock();
            size_t queue_size = _queue.size();
            bool rst = false;
            if (queue_size != 0) {
                *item = _queue.front();
                _queue.pop_front();
                rst = true;
                if (queue_size == _size - 1) {
                    _full_cond.notify();
                }
            }
            _lock.unlock();
            return rst;
        }
        
        T pop() {
            _lock.lock();
            while(_queue.empty()) {
                _empty_cond.wait();
            }

            T item = _queue.front();
            _queue.pop_front();
            if (_queue.size() == _size - 1) {
                _full_cond.notify();
            }
            _lock.unlock();
            return item;
        }

        size_t size() {
            _lock.lock();
            size_t size = _queue.size();
            _lock.unlock();
            return size;
        }

        size_t capacity() const { return _size; }
        void clear() { _queue.clear(); }

    private:
        std::deque<T> _queue;
        Mutex _lock;
        Condition _empty_cond;
        Condition _full_cond;
        size_t _size;
};


}

#endif
