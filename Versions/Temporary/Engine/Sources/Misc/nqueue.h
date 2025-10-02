/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
 *
 * Copyright (c) 1999 
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted 
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#ifndef __NSTL_QUEUE_H__
#define __NSTL_QUEUE_H__

#pragma once
#pragma warning ( disable : 4180 )

#ifndef __NSTL_HEAP_H__
#include "nheap.h"
#endif // __NSTL_HEAP_H__

namespace nstl
{

template <class _Tp, class _Sequence = vector<_Tp>, class _Compare = less() >
class priority_queue 
{
public:
  typedef _Tp      value_type;
//  typedef typename _Sequence::size_type       size_type;
  typedef          _Sequence                  container_type;

  typedef typename _Sequence::reference       reference;
  typedef typename _Sequence::const_reference const_reference;
protected:
  _Sequence c;
  _Compare comp;
public:
	int operator&( IBinSaver &f ) { f.Add( 1, &c ); return 0; }
  priority_queue() : c() {}
  explicit priority_queue(const _Compare& __x) :  c(), comp(__x) {}
  priority_queue(const _Compare& __x, const _Sequence& __s) 
    : c(__s), comp(__x)
    { make_heap(c.begin(), c.end(), comp); }

  template <class _InputIterator>
  priority_queue(_InputIterator __first, _InputIterator __last) 
    : c(__first, __last) { make_heap(c.begin(), c.end(), comp); }

  template <class _InputIterator>
  priority_queue(_InputIterator __first, 
                 _InputIterator __last, const _Compare& __x)
    : c(__first, __last), comp(__x)
    { make_heap(c.begin(), c.end(), comp); }

  template <class _InputIterator>
  priority_queue(_InputIterator __first, _InputIterator __last,
                 const _Compare& __x, const _Sequence& __s)
  : c(__s), comp(__x)
  { 
    c.insert(c.end(), __first, __last);
    make_heap(c.begin(), c.end(), comp);
  }

  bool empty() const { return c.empty(); }
  //size_type size() const { return c.size(); }
  int size() const { return c.size(); }
  const_reference top() const { return c.front(); }
  void push(const value_type& __x) 
	{
    c.push_back(__x); 
    push_heap(c.begin(), c.end(), comp);
  }
  void pop() 
	{
    pop_heap(c.begin(), c.end(), comp);
    c.pop_back();
  }
	void reserve( int count )
	{
		c.reserve( count );
	}
	void clear() { c.clear(); }
};

} // end of namespace nstl

#endif // __NSTL_QUEUE_H__
