/*
MIT License

Copyright (c) 2024 Pavel Slama

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef MOVINGAVERAGE_H
#define MOVINGAVERAGE_H

#include <inttypes.h>

#if defined(__MBED__)
  #include "mbed.h"
#endif

template <class T, uint16_t N>
class MovingAverage {
 public:
  MovingAverage();
  virtual ~MovingAverage(void);

  T add(T value);
  T get();
  int32_t get_sum();
  void fill(T value);
  void reset();
  void set_samples(uint16_t samples);

 private:
  bool _first;
  uint8_t _next;
  uint8_t _shift;
  int32_t _sum;
  uint16_t _samples;

  T _buffer[N];
  T _result;
};

template <class T, uint16_t N>
MovingAverage<T, N>::MovingAverage():
  _first(true),
  _next(0),
  _shift(0),
  _sum(0),
  _samples(N) {
  _result = 0;

  // prevent N==0
  static_assert(N > 0, "Buffer length must be greater than 0");
  // prevent N not being a power of 2
  static_assert((N & (N - 1)) == 0, "Buffer length must be a power of 2");

  while (_samples >> _shift != 1) {
    _shift++;
  }

}

template <class T, uint16_t N>
MovingAverage<T, N>::~MovingAverage(void) {
}

template <class T, uint16_t N>
T MovingAverage<T, N>::get() {
  return _result;
}
template <class T, uint16_t N>
int32_t MovingAverage<T, N>::get_sum() {
  return _sum;
}

template <class T, uint16_t N>
T MovingAverage<T, N>::add(T value) {
  // fill buffer when using first
  if (_first) {
    _first = false;
    fill(value);

  } else {
    _sum = _sum - _buffer[_next] + value;
    _buffer[_next] = value;
    _next = (_next + 1) & (_samples - 1);
  }

  _result = (_sum + (_samples >> 1)) >> _shift;  // same as (_sum + (_samples / 2)) / _samples;

  return _result;
}

template <class T, uint16_t N>
void MovingAverage<T, N>::fill(T value) {
  for (uint16_t i = 0; i < _samples; i++) {
    _buffer[i] = value;
  }

  _sum = (int32_t)value * _samples;
  _next = 0;
}

template <class T, uint16_t N>
void MovingAverage<T, N>::reset() {
  _first = true;
}

template <class T, uint16_t N>
void MovingAverage<T, N>::set_samples(uint16_t samples) {
  if (samples <= N) {
    _samples = samples;

    reset();

    _shift = 0;

    while (_samples >> _shift != 1) {
      _shift++;
    }

    _samples = 1 << _shift; //ensure _samples is a power of 2

  }
}

#endif
