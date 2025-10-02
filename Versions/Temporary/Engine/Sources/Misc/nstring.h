/*
 * Copyright (c) 1997-1999
 * Silicon Graphics Computer Systems, Inc.
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

#pragma once

#include "nalgobase.h"
#include "nhash_fun.h"

// Standard C++ string class.  This class has performance
// characteristics very much like vector<>, meaning, for example, that
// it does not perform reference-count or copy-on-write, and that
// concatenation of two strings is an O(N) operation. 

// There are three reasons why basic_string is not identical to
// vector.  First, basic_string always stores a null character at the
// end; this makes it possible for c_str to be a fast operation.
// Second, the C++ standard requires basic_string to copy elements
// using char_traits<>::assign, char_traits<>::copy, and
// char_traits<>::move.  This means that all of vector<>'s low-level
// operations must be rewritten.  Third, basic_string<> has a lot of
// extra functions in its interface that are convenient but, strictly
// speaking, redundant.

// Additionally, the C++ standard imposes a major restriction: according
// to the standard, the character type T must be a POD type.  This
// implementation weakens that restriction, and allows T to be a
// a user-defined non-POD type.  However, T must still have a
// default constructor.

namespace nstl
{

// A helper class to use a char_traits as a function object.

/*template <class _Traits> struct _Not_within_traits
  : public unary_function<typename _Traits::char_type, bool> {
  typedef typename _Traits::char_type T;
  const T* _M_first;
  const T* _M_last;

  _Not_within_traits(const typename _Traits::char_type* __f, 
		     const typename _Traits::char_type* __l) 
    : _M_first(__f), _M_last(__l) {}

  bool operator()(const typename _Traits::char_type& __x) const {
    return find_if((T*)_M_first, (T*)_M_last, 
                   _Eq_char_bound<_Traits>(__x)) == (T*)_M_last;
  }
};*/

// ------------------------------------------------------------
// Class _String_base.  

// _String_base is a helper class that makes it it easier to write an
// exception-safe version of basic_string.  The constructor allocates,
// but does not initialize, a block of memory.  The destructor
// deallocates, but does not destroy elements within, a block of
// memory.  The destructor assumes that _M_start either is null, or else
// points to a block of memory that was allocated using _String_base's 
// allocator and whose size is _M_end_of_storage - _M_start.

template <class _Tp> class _String_base {
public:
  //_STLP_FORCE_ALLOCATORS(_Tp)
  //typedef typename _Alloc_traits<_Tp>::allocator_type allocator_type;
  _Tp*    _M_start;
  _Tp*    _M_finish;
	_Tp*    _M_end_of_storage;
  //_STLP_alloc_proxy<_Tp*, _Tp, allocator_type> _M_end_of_storage;
                                // Precondition: 0 < __n <= max_size().
  void _M_allocate_block(size_t n)
	{
		_M_start  = new _Tp[ n ];
		_M_finish = _M_start; 
		_M_end_of_storage = _M_start + n; 
	}
  //void _M_deallocate_block() 
	//{ delete[] _M_start; }//_M_end_of_storage.deallocate(_M_start, _M_end_of_storage - _M_start); }
  
  //size_t max_size() const { return (size_t(-1) / sizeof(_Tp)) - 1; }

  _String_base()
    : _M_start(0), _M_finish(0), _M_end_of_storage(0) {}
  
  _String_base(size_t __n)
//    : _M_start(0), _M_finish(0), _M_end_of_storage(0)
    { _M_allocate_block(__n); }

  ~_String_base() { delete[] _M_start; }

};


inline size_t __strlen( const char *p ) { return strlen(p); }
inline size_t __strlen( const wchar_t *p ) { return wcslen(p); }
inline int __strncmp( const char *p1, const char *p2, size_t n ) { return strncmp( p1, p2, n ); }
inline int __strncmp( const wchar_t *p1, const wchar_t *p2, size_t n ) { return wcsncmp( p1, p2, n ); }

// ------------------------------------------------------------
// Class basic_string.  

// Class invariants:
// (1) [start, finish) is a valid range.
// (2) Each iterator in [start, finish) points to a valid object
//     of type value_type.
// (3) *finish is a valid object of type value_type; in particular,
//     it is value_type().
// (4) [finish + 1, end_of_storage) is a valid range.
// (5) Each iterator in [finish + 1, end_of_storage) points to 
//     unininitialized memory.

// Note one important consequence: a string of length n must manage
// a block of memory whose size is at least n + 1.  

struct _Reserve_t {};

//template <class T> 
template <class T>
class basic_string : protected _String_base<T> 
{
private:                        // Protected members inherited from base.
  typedef _String_base<T> _Base;
  typedef basic_string<T> _Self;
  // fbp : used to optimize char/wchar_t cases, and to simplify
  // _STLP_DEFAULT_CONSTRUCTOR_BUG problem workaround
//  typedef typename _Is_integer<T>::_Integral _Char_Is_Integral;
public:
  typedef T value_type;
  //typedef _Traits traits_type;

  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
//  typedef size_t size_t;
  //typedef ptrdiff_t difference_type;
  //typedef random_access_iterator_tag _Iterator_category;

  typedef const value_type*                const_iterator;
  typedef value_type*                      iterator;

//  _STLP_DECLARE_RANDOM_ACCESS_REVERSE_ITERATORS;

//# ifdef _STLP_STATIC_CONST_INIT_BUG
  enum { npos = -1 };
//# else
//  static const size_t npos = ~(size_t)0;
//# endif

  //typedef _String_reserve_t _Reserve_t;
/*# if defined (_STLP_USE_NATIVE_STRING) && ! defined (_STLP_DEBUG)
#  if (defined(__IBMCPP__) && (500 <= __IBMCPP__) && (__IBMCPP__ < 600) )
   // this typedef is being used for conversions
   typedef typename _STLP_VENDOR_basic_string<T, 
    typename _STLP_VENDOR_allocator<T> > __std_string;
#  else
   // this typedef is being used for conversions
   typedef _STLP_VENDOR_basic_string<T, 
    _STLP_VENDOR_allocator<T> > __std_string;
#  endif
# endif
  */
public:                         // Constructor, destructor, assignment.
//  typedef typename _String_base<T>::allocator_type allocator_type;

//  allocator_type get_allocator() const {
    //return _STLP_CONVERT_ALLOCATOR((const allocator_type&)this->_M_end_of_storage, T);
//  }

	basic_string() : _String_base<T>(8)
	{
		*_M_finish=0; // _M_terminate_string();  
	}

  basic_string(_Reserve_t, size_t __n) : _String_base<T>(__n + 1) 
	{ 
		*_M_finish=0; // _M_terminate_string();  
  }

  basic_string(const basic_string<T> &__s)
	{
		_M_range_initialize(__s._M_start, __s._M_finish);  
	}

  /*basic_string(const _Self& __s, size_t __pos, size_t __n = npos) 
	{
		ASSERT( __pos <= __s.size() );
    _M_range_initialize(__s._M_start + __pos,
                          __s._M_start + __pos + (min) (__n, __s.size() - __pos));
  }*/

  basic_string(const T* __s, size_t __n)
  { 
		_M_range_initialize(__s, __s + __n); 
  }

  basic_string(const T* __s)
	{
		_M_range_initialize(__s, __s + __strlen(__s) );
	}

	basic_string(const T* __f, const T *__l)
	{
		_M_range_initialize(__f, __l );
	}

  /*basic_string(size_t __n, T __c)
    : _String_base<T>(__n + 1)
  {
    this->_M_finish = fill_n(this->_M_start, __n, __c);
		*_M_finish=0; // _M_terminate_string();  
  }*/

  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
	// _M_range_init is valid only for random access iterators
  /*template <class _InputIterator> basic_string(_InputIterator __f, _InputIterator __l)
  {
		_M_range_initialize(__f, __l);
  }*/

	/*
# if defined (_STLP_USE_NATIVE_STRING) && ! defined (_STLP_DEBUG)
  // these conversion operations still needed for
  // strstream, etc.
  basic_string (const __std_string& __x): _String_base<T>(allocator_type())
    {
      const T* __s = __x.data();
      _M_range_initialize(__s, __s + __x.size()); 
    }
  
  operator __std_string() const { return __std_string(this->data(), this->size()); }
# endif
*/
	//~basic_string() { _Destroy(this->_M_start, this->_M_finish + 1); }
    
  _Self& operator=(const _Self& __s) {
    if (&__s != this) 
      assign(__s._M_start, __s._M_finish);
    return *this;
  }

  _Self& operator=(const T* __s) { 
    return assign(__s, __s + __strlen(__s) );
  }

  _Self& operator=(T __c)
    { return assign(1, __c); }


private:                        // Helper functions used by constructors
                                // and elsewhere.
  // fbp : simplify integer types (char, wchar)
  /*void _M_construct_null_aux(T* __p, const __false_type&) {
    _Construct(__p);
  }
  void _M_construct_null_aux(T* __p, const __true_type&) {
    *__p = 0;
  }

  void _M_construct_null(T* __p) {
    _M_construct_null_aux(__p, _Char_Is_Integral());
  }*/

private:                        
  // Helper functions used by constructors.  It is a severe error for
  // any of them to be called anywhere except from within constructors.


  /*template <class _InputIter> void _M_range_initialize(_InputIter __f, _InputIter __l,
                           const input_iterator_tag &) {
    this->_M_allocate_block(8);
    _M_construct_null(this->_M_finish);
    append(__f, __l);
  }*/

/*  template <class _ForwardIter> void _M_range_initialize(_ForwardIter __f, _ForwardIter __l, 
                           const forward_iterator_tag &) {
    difference_type __n = distance(__f, __l);
    this->_M_allocate_block(__n + 1);
    this->_M_finish = copy(__f, __l, this->_M_start);
		*_M_finish=0; // _M_terminate_string();  
  }*/

  template <class _InputIter> void _M_range_initialize(_InputIter __f, _InputIter __l) {
    //_M_range_initialize(__f, __l, _STLP_ITERATOR_CATEGORY(__f, _InputIter));
		int __n = int(__l - __f);
		this->_M_allocate_block(__n + 1);
		this->_M_finish = copy( __f, __l, this->_M_start);
		*_M_finish=0; // _M_terminate_string();  
  }

public:                         // Iterators.
  iterator begin()             { return this->_M_start; }
  iterator end()               { return this->_M_finish; }
  const_iterator begin() const { return this->_M_start; }
  const_iterator end()   const { return this->_M_finish; }  

/*  reverse_iterator rbegin()             
    { return reverse_iterator(this->_M_finish); }
  reverse_iterator rend()               
    { return reverse_iterator(this->_M_start); }
  const_reverse_iterator rbegin() const 
    { return const_reverse_iterator(this->_M_finish); }
  const_reverse_iterator rend()   const 
    { return const_reverse_iterator(this->_M_start); }
*/
public:                         // Size, capacity, etc.
  int size() const { return this->_M_finish - this->_M_start; }
  int length() const { return size(); }

  //size_t max_size() const { return _Base::max_size(); }


  void resize(size_t __n, T __c) {
    if (__n <= size())
      erase(begin() + __n, end());
    else
      append(__n - size(), __c);
  }
  void resize(size_t __n) { resize(__n, 0); }

  void reserve(size_t __res_arg = 0)
	{
		size_t __n = (max)((unsigned)__res_arg, (unsigned)size()) + 1;
		pointer __new_start = new T[ __n ];
		pointer __new_finish = __new_start;

		__new_finish = copy(this->_M_start, this->_M_finish, __new_start);
		*__new_finish=0;// _M_construct_null(__new_finish);

		//_Destroy(this->_M_start, this->_M_finish + 1);
		delete[] _M_start;
		this->_M_start = __new_start;
		this->_M_finish = __new_finish;
		this->_M_end_of_storage = __new_start + __n;
	}


  int capacity() const { return (this->_M_end_of_storage - this->_M_start) - 1; }

  void clear() {
    if (!empty()) {
      *(this->_M_start) = 0;
      //_Destroy(this->_M_start+1, this->_M_finish+1);
      this->_M_finish = this->_M_start;
    }
  } 

  bool empty() const { return this->_M_start == this->_M_finish; }    

public:                         // Element access.

  const_reference operator[](int __n) const
    { 
			ASSERT( __n >= 0 && __n < size() );
			return *(this->_M_start + __n); 
		}
  reference operator[](int __n)
    { 
			ASSERT( __n >= 0 && __n < size() );
			return *(this->_M_start + __n); 
		}

public:                         // Append, operator+=, push_back.

  _Self& operator+=(const _Self& __s) { return append(__s); }
  _Self& operator+=(const T* __s) { return append(__s); }
  _Self& operator+=(T __c) { push_back(__c); return *this; }

  _Self& append(const _Self& __s) 
    { return append(__s._M_start, __s._M_finish); }

  _Self& append(const _Self& __s,
                       size_t __pos, size_t __n)
  {
		ASSERT( __pos <= __s.size() )
    return append(__s._M_start + __pos,
                  __s._M_start + __pos + (min) (__n, __s.size() - __pos));
  }

  //_Self& append(const T* __s, size_t __n) 
  //  { return append(__s, __s+__n); }
  _Self& append(const T* __s) 
	{ return append(__s, __s + __strlen(__s)); }
  _Self& append(size_t __n, T __c)
	{
		if (size() + __n > capacity())
			reserve(size() + (max)((unsigned)size(), (unsigned)__n));
		if (__n > 0) {
			fill_n(this->_M_finish + 1, __n - 1, __c);
			*(this->_M_finish + __n) = 0;//_M_construct_null(this->_M_finish + __n);
			*end() = __c;
			this->_M_finish += __n;
		}
		return *this;
	}

  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
  template <class _InputIter> 
	_Self& append(_InputIter __first, _InputIter __last) {
		for ( ; __first != __last ; ++__first)
			push_back(*__first);
		return *this;
//    typedef typename _Is_integer<_InputIter>::_Integral _Integral;
//    return _M_append_dispatch(__first, __last, _Integral());
  }

  void push_back(T __c) {
    if (this->_M_finish + 1 == this->_M_end_of_storage)
      reserve(size() + (max)((unsigned)size(), (unsigned)size_t(1)));
		*(this->_M_finish + 1)=0;// _M_construct_null(this->_M_finish + 1);
    *(this->_M_finish) = __c;
    ++this->_M_finish;
  }

  void pop_back() {
    *(this->_M_finish - 1) = 0;
    //_Destroy(this->_M_finish);
    --this->_M_finish;
  }

private:                        // Helper functions for append.

/*  template <class _InputIter> _Self& append(_InputIter __first, _InputIter __last)
  {
	  for ( ; __first != __last ; ++__first)
	    push_back(*__first);
	  return *this;
	}

  template <class _Integer> _Self& _M_append_dispatch(_Integer __n, _Integer __x, const __true_type&) {
    return append((size_t) __n, (T) __x);
  }

  template <class _InputIter> _Self& _M_append_dispatch(_InputIter __f, _InputIter __l,
                                   const __false_type&) {
    return append(__f, __l, _STLP_ITERATOR_CATEGORY(__f, _InputIter));
  }*/
public:                         // Assign
  
  _Self& assign(const _Self& __s) { return assign(__s._M_start, __s._M_finish); }

  /*_Self& assign(const _Self& __s, size_t __pos, size_t __n) 
	{
		ASSERT( __pos <= __s.size());
    return assign(__s._M_start + __pos, 
                  __s._M_start + __pos + (min) (__n, __s.size() - __pos));
  }*/

  _Self& assign(const T* __s, size_t __n) { return assign(__s, __s + __n); }

	_Self& assign(const T* __s) { return assign(__s, __s + __strlen(__s));}

  //_Self& assign(size_t __n, T __c);


private:                        // Helper functions for assign.

//  template <class _Integer> 
//  _Self& _M_assign_dispatch(_Integer __n, _Integer __x, const __true_type&) {
    //return assign((size_t) __n, (T) __x);
  //}

/*  template <class _InputIter> 
  _Self& _M_assign_dispatch(_InputIter __f, _InputIter __l,
			    const __false_type&)  {
    pointer __cur = this->_M_start;
    while (__f != __l && __cur != this->_M_finish) {
      *__cur = *__f;
      ++__f;
      ++__cur;
    }
    if (__f == __l)
      erase(__cur, end());
    else
      append(__f, __l);
    return *this;
  }
  */
public:
  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
  template <class _InputIter> _Self& assign(_InputIter __f, _InputIter __l) {
    //typedef typename _Is_integer<_InputIter>::_Integral _Integral;
    //return _M_assign_dispatch(__first, __last, _Integral());
		pointer __cur = this->_M_start;
		while (__f != __l && __cur != this->_M_finish) {
			*__cur = *__f;
			++__f;
			++__cur;
		}
		if (__f == __l)
			erase(__cur, end());
		else
			append(__f, __l);
		return *this;
  }

  // if member templates are on, this works as specialization 
  _Self& assign(const T* __f, const T* __l)
  {
    ptrdiff_t __n = __l - __f;
    if (size_t(__n) <= size()) {
			memcpy( this->_M_start, __f, sizeof(T) * __n );
      //_Traits::copy(this->_M_start, __f, __n);
      erase(begin() + __n, end());
    }
    else {
			memcpy( this->_M_start, __f, sizeof(T) * size() );
      //_Traits::copy(this->_M_start, __f, size());
      append(__f + size(), __l);
    }
    return *this;
  }
  
public:                         // Insert

  _Self& insert(size_t __pos, const _Self& __s) {
		ASSERT( __pos <= size())
    insert(begin() + __pos, __s._M_start, __s._M_finish);
    return *this;
  }

  _Self& insert(size_t __pos, const _Self& __s,
                       size_t __beg, size_t __n) {
		ASSERT(__pos <= size() && __beg <= __s.size())
    size_t __len = (min) (__n, __s.size() - __beg);
    insert(begin() + __pos,
           __s._M_start + __beg, __s._M_start + __beg + __len);
    return *this;
  }

  /*_Self& insert(size_t __pos, const T* __s, size_t __n) {
    ASSERT( __pos <= size());
    insert(begin() + __pos, __s, __s + __n);
    return *this;
  }*/

  _Self& insert(size_t __pos, const T* __s) {
    ASSERT( __pos <= size());
    size_t __len = __strlen(__s);
    insert(this->_M_start + __pos, __s, __s + __len);
    return *this;
  }
    
  /*_Self& insert(size_t __pos, size_t __n, T __c) {
    ASSERT( __pos <= size());
    insert(begin() + __pos, __n, __c);
    return *this;
  }*/

  iterator insert(iterator __p, T __c) {
    if (__p == end()) {
      push_back(__c);
      return this->_M_finish - 1;
    }
    else
      return _M_insert_aux(__p, __c);
  }

  //void insert(iterator __p, size_t __n, T __c);


  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
  template <class _InputIter> void insert(iterator __p, _InputIter __first, _InputIter __last) {
		for ( ; __first != __last; ++__first) {
			__p = insert(__p, *__first);
			++__p;
		}
//    typedef typename _Is_integer<_InputIter>::_Integral _Integral;
    //_M_insert_dispatch(__p, __first, __last, _Integral());
  }


private:                        // Helper functions for insert.
/*
  template <class _InputIter> void insert(iterator __p, _InputIter __first, _InputIter __last,
	      const input_iterator_tag &)
  {
	  for ( ; __first != __last; ++__first) {
	    __p = insert(__p, *__first);
	    ++__p;
	  }
	}

  template <class _ForwardIter> void insert(iterator __position, _ForwardIter __first, _ForwardIter __last, 
	      const forward_iterator_tag &)  {
    if (__first != __last) {
      difference_type __n = distance(__first, __last);
      if (this->_M_end_of_storage - this->_M_finish >= __n + 1) {
	const difference_type __elems_after = this->_M_finish - __position;
	pointer __old_finish = this->_M_finish;
	if (__elems_after >= __n) {
	  copy((this->_M_finish - __n) + 1, this->_M_finish + 1,
			     this->_M_finish + 1);
	  this->_M_finish += __n;
		memmove( __position + __n, __position, sizeof(T) * ((__elems_after - __n) + 1) );
	  //_Traits::move(__position + __n, __position, (__elems_after - __n) + 1);
	  _M_copy(__first, __last, __position);
	      }
	else {
	  _ForwardIter __mid = __first;
	  advance(__mid, __elems_after + 1);
	  copy(__mid, __last, this->_M_finish + 1);
	  this->_M_finish += __n - __elems_after;
         copy(__position, __old_finish + 1, this->_M_finish);
         this->_M_finish += __elems_after;
	        _M_copy(__first, __mid, __position);
	}
      }
      else {
	const size_t __old_size = size();        
	const size_t __len
	  = __old_size + (max)((unsigned)__old_size, (unsigned)size_t(__n)) + 1;
	      pointer __new_start = new T[__len];
	      pointer __new_finish = __new_start;
        __new_finish = copy(this->_M_start, __position, __new_start);
        __new_finish = copy(__first, __last, __new_finish);
        __new_finish = copy(__position, this->_M_finish, __new_finish);
        *__new_finish = 0;//_M_construct_null(__new_finish);
	      //_Destroy(this->_M_start, this->_M_finish + 1);
	      delete[] _M_start;
	      this->_M_start = __new_start;
	      this->_M_finish = __new_finish;
	      this->_M_end_of_storage = __new_start + __len; 
	    }
    }
  }
*/
  /*template <class _Integer> void _M_insert_dispatch(iterator __p, _Integer __n, _Integer __x,
                          const __true_type&) {
    insert(__p, (size_t) __n, (T) __x);
  }

  template <class _InputIter> void _M_insert_dispatch(iterator __p, _InputIter __first, _InputIter __last,
                          const __false_type&) {
    insert(__p, __first, __last, _STLP_ITERATOR_CATEGORY(__first, _InputIter));
  }*/

/*  template <class _InputIterator> void 
  _M_copy(_InputIterator __first, _InputIterator __last, pointer __result) {
    for ( ; __first != __last; ++__first, ++__result)
      *__result = *__first;
  }*/

  pointer _M_insert_aux(pointer __p, T __c )
	{
		pointer __new_pos = __p;
		if (this->_M_finish + 1 < this->_M_end_of_storage) 
		{
			this->_M_finish[1] = 0;//_M_construct_null(this->_M_finish + 1);
			memmove( __p + 1, __p, sizeof(T) * (this->_M_finish - __p) );
			//_Traits::move(__p + 1, __p, this->_M_finish - __p);
			*__p = __c;//_Traits::assign(*__p, __c);
			++this->_M_finish;
		}
		else 
		{
			const size_t __old_len = size();
			const size_t __len = __old_len + (max)((unsigned)__old_len, (unsigned)size_t(1)) + 1;
			pointer __new_start = new T[__len];
			pointer __new_finish = __new_start;
			__new_pos = copy(this->_M_start, __p, __new_start);
			*__new_pos = __c;
			__new_finish = __new_pos + 1;
			__new_finish = copy(__p, this->_M_finish, __new_finish);
			*__new_finish = 0;//_M_construct_null(__new_finish);
			//_Destroy(this->_M_start, this->_M_finish + 1);
			delete[] _M_start;
			this->_M_start = __new_start;
			this->_M_finish = __new_finish;
			this->_M_end_of_storage = __new_start + __len;
		}
		return __new_pos;
	}

  /*void 
  _M_copy(const T* __first, const T* __last, T* __result) {
		memcpy( __result, __first, sizeof(T) * ( __last - __first ) );
    //_Traits::copy(__result, __first, __last - __first);
  }*/

public:                         // Erase.

  _Self& erase(size_t __pos = 0, size_t __n = npos) {
    ASSERT( __pos <= size());
    erase(begin() + __pos, begin() + __pos + (min) (__n, size() - __pos));
    return *this;
  }  

  iterator erase(iterator __position) {
                                // The move includes the terminating T().
		memmove( __position, __position + 1, sizeof(T) * (this->_M_finish - __position) );
    //_Traits::move(__position, __position + 1, this->_M_finish - __position);
    //_Destroy(this->_M_finish);
    --this->_M_finish;
    return __position;
  }

  iterator erase(iterator __first, iterator __last) {
    if (__first != __last) {
                                // The move includes the terminating T().
			memmove( __first, __last, sizeof(T) * ((this->_M_finish - __last) + 1) );
      //traits_type::move(__first, __last, (this->_M_finish - __last) + 1);
      pointer __new_finish = this->_M_finish - (__last - __first);
      //_Destroy(__new_finish + 1, this->_M_finish + 1);
      this->_M_finish = __new_finish;
    }
    return __first;
  }

public:                         // Replace.  (Conceptually equivalent
                                // to erase followed by insert.)
  _Self& replace(size_t __pos, size_t __n, const _Self& __s) {
    ASSERT( __pos <= size());
    const size_t __len = (min) (__n, size() - __pos);
    return replace(begin() + __pos, begin() + __pos + __len, 
                   __s._M_start, __s._M_finish);
  }

  _Self& replace(size_t __pos1, size_t __n1,
                        const _Self& __s,
                        size_t __pos2, size_t __n2) {
    ASSERT( __pos1 <= size() && __pos2 <= __s.size());
    const size_t __len1 = (min) (__n1, size() - __pos1);
    const size_t __len2 = (min) (__n2, __s.size() - __pos2);
    return replace(begin() + __pos1, begin() + __pos1 + __len1,
                   __s._M_start + __pos2, __s._M_start + __pos2 + __len2);
  }

  _Self& replace(size_t __pos, size_t __n1,
                        const T* __s, size_t __n2) {
    ASSERT(__pos <= size());
    const size_t __len = (min) (__n1, size() - __pos);
    return replace(begin() + __pos, begin() + __pos + __len,
                   __s, __s + __n2);
  }

  _Self& replace(size_t __pos, size_t __n1,
                        const T* __s) {
    ASSERT( __pos <= size());
    const size_t __len = (min) (__n1, size() - __pos);
    const size_t __n2 = __strlen(__s);
    return replace(begin() + __pos, begin() + __pos + __len,
                   __s, __s + __strlen(__s));
  }

  _Self& replace(size_t __pos, size_t __n1,
                        size_t __n2, T __c) {
    ASSERT(__pos <= size())
    const size_t __len = (min) (__n1, size() - __pos);
    return replace(begin() + __pos, begin() + __pos + __len, __n2, __c);
  }

  _Self& replace(iterator __first, iterator __last, 
                        const _Self& __s) 
    { return replace(__first, __last, __s._M_start, __s._M_finish); }

  _Self& replace(iterator __first, iterator __last,
                        const T* __s, size_t __n) 
    { return replace(__first, __last, __s, __s + __n); }

  _Self& replace(iterator __first, iterator __last,
                        const T* __s) {
    return replace(__first, __last, __s, __s + __strlen(__s));
  }

  _Self& replace(iterator __first, iterator __last, 
                        size_t __n, T __c);

  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
  template <class _InputIter> _Self& replace(iterator __first, iterator __last,
                        _InputIter __f, _InputIter __l) {
//    typedef typename _Is_integer<_InputIter>::_Integral _Integral;
//    return _M_replace_dispatch(__first, __last, __f, __l,  _Integral());
		for ( ; __first != __last && __f != __l; ++__first, ++__f)
			*__first = *__f;

		if (__f == __l)
			erase(__first, __last);
		else
			insert(__last, __f, __l);
		return *this;
 }

private:                        // Helper functions for replace.


/*  template <class _Integer> _Self& _M_replace_dispatch(iterator __first, iterator __last,
                                    _Integer __n, _Integer __x,
                                    const __true_type&) {
    return replace(__first, __last, (size_t) __n, (T) __x);
  }

  template <class _InputIter> _Self& _M_replace_dispatch(iterator __first, iterator __last,
                                    _InputIter __f, _InputIter __l,
                                    const __false_type&) {
    return replace(__first, __last, __f, __l, _STLP_ITERATOR_CATEGORY(__f, _InputIter));
  }*/

/*  template <class _InputIter> _Self& replace(iterator __first, iterator __last,
                        _InputIter __f, _InputIter __l, const input_iterator_tag &)  {
	  for ( ; __first != __last && __f != __l; ++__first, ++__f)
	    *__first = *__f;

	  if (__f == __l)
	    erase(__first, __last);
	  else
	    insert(__last, __f, __l);
	  return *this;
	}

  template <class _ForwardIter> _Self& replace(iterator __first, iterator __last,
                        _ForwardIter __f, _ForwardIter __l, 
                        const forward_iterator_tag &)  {
	  difference_type __n = distance(__f, __l);
	  const difference_type __len = __last - __first;
	  if (__len >= __n) {
	    _M_copy(__f, __l, __first);
	    erase(__first + __n, __last);
	  }
	  else {
	    _ForwardIter __m = __f;
	    advance(__m, __len);
	    _M_copy(__f, __m, __first);
	    insert(__last, __m, __l);
	  }
	  return *this;
	}
*/
public:                         // Other modifier member functions.

  /*size_t copy(T* __s, size_t __n, size_t __pos = 0) const {
    ASSERT(__pos <= size());
    const size_t __len = (min) (__n, size() - __pos);
		memcpy( __s, this->_M_start + __pos, __len);
    //_Traits::copy(__s, this->_M_start + __pos, __len);
    return __len;
  }*/

  void swap(_Self& __s) {
    nstl::swap(this->_M_start, __s._M_start);
    nstl::swap(this->_M_finish, __s._M_finish);
    nstl::swap(this->_M_end_of_storage, __s._M_end_of_storage);
  }

public:                         // Conversion to C string.

  const T* c_str() const { return this->_M_start; }
  const T* data()  const { return this->_M_start; }

public:                         // find.

  size_t find(const _Self& __s, size_t __pos = 0) const 
    { return find(__s._M_start, __pos, __s.size()); }

  size_t find(const T* __s, size_t __pos = 0) const 
    { return find(__s, __pos, __strlen(__s)); }

  size_t find(const T* __s, size_t __pos, size_t __n) const;
  size_t find(T __c, size_t __pos = 0) const;

public:                         // rfind.
/*
  size_t rfind(const _Self& __s, size_t __pos = npos) const 
    { return rfind(__s._M_start, __pos, __s.size()); }

  size_t rfind(const T* __s, size_t __pos = npos) const 
    { return rfind(__s, __pos, __strlen(__s)); }

  size_t rfind(const T* __s, size_t __pos, size_t __n) const;
*/
  size_t rfind(T __c, size_t __pos = npos) const;
public:                         // find_first_of
  
  size_t find_first_of(const _Self& __s, size_t __pos = 0) const 
    { return find_first_of(__s._M_start, __pos, __s.size()); }

  size_t find_first_of(const T* __s, size_t __pos = 0) const 
    { return find_first_of(__s, __pos, __strlen(__s)); }

  size_t find_first_of(const T* __s, size_t __pos, 
                          size_t __n) const;

  size_t find_first_of(T __c, size_t __pos = 0) const 
    { return find(__c, __pos); }

public:                         // find_last_of

 /* size_t find_last_of(const _Self& __s,
                         size_t __pos = npos) const
    { return find_last_of(__s._M_start, __pos, __s.size()); }

  size_t find_last_of(const T* __s, size_t __pos = npos) const 
    { return find_last_of(__s, __pos, __strlen(__s)); }

  size_t find_last_of(const T* __s, size_t __pos, 
                         size_t __n) const;

  size_t find_last_of(T __c, size_t __pos = npos) const {
    return rfind(__c, __pos);
  }
*/
public:                         // find_first_not_of

  size_t find_first_not_of(const _Self& __s, 
                              size_t __pos = 0) const 
    { return find_first_not_of(__s._M_start, __pos, __s.size()); }

  size_t find_first_not_of(const T* __s, size_t __pos = 0) const 
    { return find_first_not_of(__s, __pos, __strlen(__s)); }

  size_t find_first_not_of(const T* __s, size_t __pos,
                              size_t __n) const;

  size_t find_first_not_of(T __c, size_t __pos = 0) const;

public:                         // find_last_not_of

  size_t find_last_not_of(const _Self& __s, 
                             size_t __pos = npos) const
    { return find_last_not_of(__s._M_start, __pos, __s.size()); }

  size_t find_last_not_of(const T* __s, size_t __pos = npos) const
    { return find_last_not_of(__s, __pos,__strlen(__s)); }

  size_t find_last_not_of(const T* __s, size_t __pos,
                             size_t __n) const;

  size_t find_last_not_of(T __c, size_t __pos = npos) const;

public:                         // Substring.

  _Self substr(size_t __pos = 0, size_t __n = npos) const {
    ASSERT(__pos <= size());
    return _Self(this->_M_start + __pos, 
                        this->_M_start + __pos + (min) (__n, size() - __pos));
  }

public:                         // Compare

  int compare(const _Self& __s) const 
    { return _M_compare(this->_M_start, this->_M_finish, __s._M_start, __s._M_finish); }

  int compare(size_t __pos1, size_t __n1,
              const _Self& __s) const {
    ASSERT(__pos1 <= size());
    return _M_compare(this->_M_start + __pos1, 
                      this->_M_start + __pos1 + (min) (__n1, size() - __pos1),
                      __s._M_start, __s._M_finish);
  }
    
  int compare(size_t __pos1, size_t __n1,
              const _Self& __s,
              size_t __pos2, size_t __n2) const {
    ASSERT(__pos1 <= size() && __pos2 <= __s.size());
    return _M_compare(this->_M_start + __pos1, 
                      this->_M_start + __pos1 + (min) (__n1, size() - __pos1),
                      __s._M_start + __pos2, 
                      __s._M_start + __pos2 + (min) (__n2, __s.size() - __pos2));
  }

  int compare(const T* __s) const {
    return _M_compare(this->_M_start, this->_M_finish, __s, __s + __strlen(__s));
  }

  int compare(size_t __pos1, size_t __n1, const T* __s) const {
    ASSERT(__pos1 <= size());
    return _M_compare(this->_M_start + __pos1, 
                      this->_M_start + __pos1 + (min) (__n1, size() - __pos1),
                      __s, __s + __strlen(__s));
  }

  int compare(size_t __pos1, size_t __n1, const T* __s,
              size_t __n2) const {
    ASSERT(__pos1 <= size());
    return _M_compare(this->_M_start + __pos1, 
                      this->_M_start + __pos1 + (min) (__n1, size() - __pos1),
                      __s, __s + __n2);
  }

public:                        // Helper functions for compare.
  
  static int  _M_compare(const T* __f1, const T* __l1,
                        const T* __f2, const T* __l2) {
    const ptrdiff_t __n1 = __l1 - __f1;
    const ptrdiff_t __n2 = __l2 - __f2;
    const int cmp = __strncmp(__f1, __f2, (min) (__n1, __n2));
    return cmp != 0 ? cmp : (__n1 < __n2 ? -1 : (__n1 > __n2 ? 1 : 0));
  }
};


// ------------------------------------------------------------
// Non-member functions.

template <class T> inline basic_string<T> 
operator+(const basic_string<T>& __s,
          const basic_string<T>& __y)
{
  typedef basic_string<T> _Str;
  //typedef typename _Str::_Reserve_t _Reserve_t;
  _Str __result(_Reserve_t(), __s.size() + __y.size());
  __result.append(__s);
  __result.append(__y);
  return __result;
}

template <class T> inline basic_string<T> 
operator+(const T* __s,
          const basic_string<T>& __y) {
  typedef basic_string<T> _Str;
  //typedef typename _Str::_Reserve_t _Reserve_t;
  const size_t __n = __strlen(__s);
  _Str __result(_Reserve_t(), __n + __y.size());
  __result.append(__s, __s + __n);
  __result.append(__y);
  return __result;
}

template <class T> inline basic_string<T> 
operator+(T __c,
          const basic_string<T>& __y) {
  typedef basic_string<T> _Str;
  //typedef typename _Str::_Reserve_t _Reserve_t;
  _Str __result(_Reserve_t(), 1 + __y.size());
  __result.push_back(__c);
  __result.append(__y);
  return __result;
}

template <class T> inline basic_string<T> 
operator+(const basic_string<T>& __x,
          const T* __s) {
  typedef basic_string<T> _Str;
  //typedef typename _Str::_Reserve_t _Reserve_t;
  const size_t __n = __strlen(__s);
  _Str __result(_Reserve_t(), __x.size() + __n);
  __result.append(__x);
  __result.append(__s, __s + __n);
  return __result;
}

template <class T> inline basic_string<T> 
operator+(const basic_string<T>& __x,
          const T __c) {
  typedef basic_string<T> _Str;
  //typedef typename _Str::_Reserve_t _Reserve_t;
  _Str __result(_Reserve_t(), __x.size() + 1);
  __result.append(__x);
  __result.push_back(__c);
  return __result;
}


// Operator== and operator!=

template <class T> inline bool 
operator==(const basic_string<T>& __x,
           const basic_string<T>& __y) {
  return __x.size() == __y.size() && __strncmp(__x.data(), __y.data(), __x.size()) == 0;
}

template <class T> inline bool 
operator==(const T* __s,
           const basic_string<T>& __y) {
  size_t __n = __strlen(__s);
  return __n == __y.size() && __strncmp(__s, __y.data(), __n) == 0;
}

template <class T> inline bool 
operator==(const basic_string<T>& __x,
           const T* __s) {
  size_t __n = __strlen(__s);
  return __x.size() == __n && __strncmp(__x.data(), __s, __n) == 0;
}

// Operator< (and also >, <=, and >=).

template <class T> inline bool 
operator<(const basic_string<T>& __x,
          const basic_string<T>& __y) {
  return basic_string<T> ::_M_compare(__x.begin(), __x.end(), 
		 __y.begin(), __y.end()) < 0;
}

template <class T> inline bool 
operator<(const T* __s,
          const basic_string<T>& __y) {
  size_t __n = __strlen(__s);
  return basic_string<T> ::_M_compare(__s, __s + __n, __y.begin(), __y.end()) < 0;
}

template <class T> inline bool 
operator<(const basic_string<T>& __x,
          const T* __s) {
  size_t __n = __strlen(__s);
  return basic_string<T> ::_M_compare(__x.begin(), __x.end(), __s, __s + __n) < 0;
}

template <class T> inline bool 
operator!=(const basic_string<T>& __x,
           const basic_string<T>& __y) {
  return !(__x == __y);
}

template <class T> inline bool 
operator>(const basic_string<T>& __x,
          const basic_string<T>& __y) {
  return __y < __x;
}

template <class T> inline bool 
operator<=(const basic_string<T>& __x,
           const basic_string<T>& __y) {
  return !(__y < __x);
}

template <class T> inline bool 
operator>=(const basic_string<T>& __x,
           const basic_string<T>& __y) {
  return !(__x < __y);
}

template <class T> inline bool  
operator!=(const T* __s,
           const basic_string<T>& __y) {
  return !(__s == __y);
}

template <class T> inline bool  
operator!=(const basic_string<T>& __x,
           const T* __s) {
  return !(__x == __s);
}

template <class T> inline bool 
operator>(const T* __s,
          const basic_string<T>& __y) {
  return __y < __s;
}

template <class T> inline bool 
operator>(const basic_string<T>& __x,
          const T* __s) {
  return __s < __x;
}

template <class T> inline bool 
operator<=(const T* __s,
           const basic_string<T>& __y) {
  return !(__y < __s);
}

template <class T> inline bool 
operator<=(const basic_string<T>& __x,
           const T* __s) {
  return !(__s < __x);
}

template <class T> inline bool 
operator>=(const T* __s,
           const basic_string<T>& __y) {
  return !(__s < __y);
}

template <class T> inline bool 
operator>=(const basic_string<T>& __x,
           const T* __s) {
  return !(__x < __s);
}


template <class T> inline void 
swap(basic_string<T>& __x,
     basic_string<T>& __y) {
  __x.swap(__y);
}

///template <class T> void   _S_string_copy(const basic_string<T>& __s, T* __buf, size_t __n);

# undef basic_string

/*
# if defined (_STLP_NESTED_TYPE_PARAM_BUG)
#  define size_t size_t
#  define size_t size_t
#  define iterator   T*
# else
#  define size_t _STLP_TYPENAME_ON_RETURN_TYPE basic_string<T>::size_t
# endif
*/

// ------------------------------------------------------------
// Non-inline declarations.


// Change the string's capacity so that it is large enough to hold
//  at least __res_arg elements, plus the terminating T().  Note that,
//  if __res_arg < capacity(), this member function may actually decrease
//  the string's capacity.
/*
template <class T> basic_string<T>& basic_string<T>::append(const T* __first, const T* __last)
{
  if (__first != __last) {
    const size_t __old_size = size();
    ptrdiff_t __n = __last - __first;
    if (__old_size + __n > capacity()) {
      const size_t __len = __old_size + (max)((unsigned)__old_size, (unsigned)(size_t) __n) + 1;
      pointer __new_start = new T[__len];
      pointer __new_finish = __new_start;
      __new_finish = copy(this->_M_start, this->_M_finish, __new_start);
      __new_finish = copy(__first, __last, __new_finish);
      *__new_finish = 0;///_M_construct_null(__new_finish);
      //_Destroy(this->_M_start, this->_M_finish + 1);
      delete[] _M_start;
      this->_M_start = __new_start;
      this->_M_finish = __new_finish;
      this->_M_end_of_storage = __new_start + __len; 
    }
    else {
      const T* __f1 = __first;
      ++__f1;
      copy(__f1, __last, this->_M_finish + 1);
      this->_M_finish[__n] = 0;//_M_construct_null(this->_M_finish + __n);
      *end() = *__first;
      this->_M_finish += __n;
    }
  }
  return *this;  
}
*/
/*template <class T> 
basic_string<T>& 
basic_string<T>::assign(size_t __n, T __c) {
  if (__n <= size()) {
		fill_n( this->_M_start, __n, __c );
    //_Traits::assign(this->_M_start, __n, __c);
    erase(begin() + __n, end());
  }
  else {
		fill_n( this->_M_start, size(), __c );
    //_Traits::assign(this->_M_start, size(), __c);
    append(__n - size(), __c);
  }
  return *this;
}
*/
/*
template <class T> T* 
basic_string<T> ::_M_insert_aux(T* __p, T __c)
{
}
*/
/*template <class T> void basic_string<T>::insert(iterator __position,
           size_t __n, T __c)
{
  if (__n != 0) {
    if (size_t(this->_M_end_of_storage - this->_M_finish) >= __n + 1) {
      const size_t __elems_after = this->_M_finish - __position;
      pointer __old_finish = this->_M_finish;
      if (__elems_after >= __n) {
        copy((this->_M_finish - __n) + 1, this->_M_finish + 1,
                           this->_M_finish + 1);
        this->_M_finish += __n;
				memmove( __position + __n, __position, sizeof(T) * ( (__elems_after - __n) + 1 ) );
        //_Traits::move(__position + __n, __position, (__elems_after - __n) + 1);
        fill_n( __position, __n, __c );//_Traits::assign(__position, __n, __c);
      }
      else {
        fill_n(this->_M_finish + 1, __n - __elems_after - 1, __c);
        this->_M_finish += __n - __elems_after;
        copy(__position, __old_finish + 1, this->_M_finish);
        this->_M_finish += __elems_after;
        fill_n( __position, __elems_after + 1, __c );//_Traits::assign(__position, __elems_after + 1, __c);
      }
    }
    else {
      const size_t __old_size = size();        
      const size_t __len = __old_size + (max)((unsigned)__old_size, (unsigned)__n) + 1;
      pointer __new_start = new T[__len];
      pointer __new_finish = __new_start;
      __new_finish = copy(this->_M_start, __position, __new_start);
      __new_finish = fill_n(__new_finish, __n, __c);
      __new_finish = copy(__position, this->_M_finish,
                                        __new_finish);
      *__new_finish = 0;//_M_construct_null(__new_finish);
      //_Destroy(this->_M_start, this->_M_finish + 1);
      delete[] _M_start;
      this->_M_start = __new_start;
      this->_M_finish = __new_finish;
      this->_M_end_of_storage = __new_start + __len;    
    }
  }
}
*/

template <class T> basic_string<T>& basic_string<T> ::replace(iterator __first, iterator __last, size_t __n, T __c)
{
  size_t __len = (size_t)(__last - __first);
  
  if (__len >= __n) {
    fill_n( __first, __n, __c );//_Traits::assign(__first, __n, __c);
    erase(__first + __n, __last);
  }
  else {
    fill_n( __first, __len, __c );//_Traits::assign(__first, __len, __c);
    insert(__last, __n - __len, __c);
  }
  return *this;
}

struct Eq
{
	template<class T>
		bool operator()( const T &a, const T &b ) const { return a == b; }
};

template <class T> size_t
basic_string<T> ::find(const T* __s, size_t __pos, size_t __n) const 
{
  if (__pos + __n > size())
    return npos;
  else {
    const const_pointer __result =
      nstl::search((const T*)this->_M_start + __pos, (const T*)this->_M_finish, 
			__s, __s + __n, Eq() );
    return __result != this->_M_finish ? __result - this->_M_start : npos;
  }
}

template <class T> size_t
basic_string<T> ::find(T __c, size_t __pos) const 
{
  if (__pos >= size())
    return npos;
  else {
    const const_pointer __result =
      nstl::find((const T*)this->_M_start + __pos, (const T*)this->_M_finish, __c);
    return __result != this->_M_finish ? __result - this->_M_start : npos;
  }
}    
/*
template <class T> size_t
basic_string<T> ::rfind(const T* __s, size_t __pos, size_t __n) const 
{
  const size_t __len = size();

  if (__n > __len)
    return npos;
  else if (__n == 0)
    return (min) (__len, __pos);
  else {
    const_pointer __last = this->_M_start + (min) (__len - __n, __pos) + __n;
    const_pointer __result = nstl::find_end((const_pointer)this->_M_start, __last,
						 __s, __s + __n,
						 _Eq_traits<_Traits>());
    return __result != __last ? __result - this->_M_start : npos;
  }
}

*/
template <class	T> 
size_t basic_string<T>::rfind(T __c, size_t __pos)	const 
{
	const size_t __len = size();

	if ( __len < 1 )
		return size_t(npos);
	else 
	{
		const const_iterator __first = begin();
		const_iterator __end = __first + (__pos == npos ? __len : __pos) - 1;
		while ( __end >= __first )
		{
			if ( *__end	== __c )
				return __end - __first;
			--__end;
		}
		//const const_iterator __last	= end() + (min)(int(__len) - 1, int(__pos)) +	1;
		//const_iterator __end = end() - 1;
		//while (	__end >= __last	) 
		//{
		//	if ( *__end	== __c ) 
		//		return __end - begin();
		//	--__end;
		//}
		return size_t(npos);
	}
}

template <class T> size_t
basic_string<T> ::find_first_of(const T* __s, size_t __pos, size_t __n) const
{
  if (__pos >= size())
    return npos;
  else {
    const_iterator __result = __find_first_of(begin() + __pos, end(),
                                              __s, __s + __n);
    return __result != end() ? __result - begin() : npos;
  }
}

/*
template <class T> size_t
basic_string<T> ::find_last_of(const T* __s, size_t __pos, size_t __n) const
{
  const size_t __len = size();

  if (__len < 1)
    return npos;
  else {
    const const_iterator __last = begin() + (min) (__len - 1, __pos) + 1;
    const const_reverse_iterator __rresult =
      __find_first_of(const_reverse_iterator(__last), rend(),
                      __s, __s + __n,
                      _Eq_traits<_Traits>());
    return __rresult != rend() ? (__rresult.base() - 1) - begin() : npos;
  }
}
*/

template<class T>
struct _Not_within_traits
{
	const T *f, *l;
	_Not_within_traits( const T *_f, const T *_l ) : f(_f), l(_l) {}
	bool operator()( T c ) { return find( f, l, c ) == l; }
};

template <class T> size_t
basic_string<T> ::find_first_not_of(const T* __s, size_t __pos, size_t __n) const
{
//  typedef typename _Traits::char_type Type;
  if (__pos > size())
    return size_t(npos);
  else {
    const_pointer __result = nstl::find_if((const T*)this->_M_start + __pos, 
				      (const T*)this->_M_finish,
                                _Not_within_traits<T>((const T*)__s, (const T*)__s + __n));
    return __result != this->_M_finish ? __result - this->_M_start : npos;
  }
}

template<class T>
struct _Neq_char_bound
{
	T c;
	_Neq_char_bound( T _c ) : c(_c) {}
	bool operator()( const T a ) { return a != c; }
};

template <class T> size_t
basic_string<T> ::find_first_not_of(T __c, size_t __pos) const
{
  if (__pos > size())
    return size_t(npos);
  else {
    const_pointer __result = nstl::find_if((const T*)this->_M_start + __pos, (const T*)this->_M_finish,
						_Neq_char_bound<T>(__c));
    return __result != this->_M_finish ? __result - this->_M_start : npos;
  }
} 




template <class T> size_t
basic_string<T> ::find_last_not_of(const T* __s, size_t __pos, size_t __n) const 
{
  const size_t __len = size();

  if (__len < 1)
    return npos;
  else {
    const_iterator __last = begin() + (min) (__len - 1, __pos) + 1;
		_Not_within_traits<T> test((const T*)__s, (const T*)__s + __n);
		while ( __last != begin() )
		{
			--__last;
			if ( test(*__last) )
				return __last - begin();
		}
		return npos;
/*    const_reverse_iterator __rlast = const_reverse_iterator(__last);
    const_reverse_iterator __rresult =
      nstl::find_if(__rlast, rend(),
			 _Not_within_traits<_Traits>((const Type*)__s, 
						     (const Type*)__s + __n));
    return __rresult != rend() ? (__rresult.base() - 1) - begin() : npos;*/
  }
}

template <class T> size_t
basic_string<T> ::find_last_not_of(T __c, size_t __pos) const 
{
  const size_t __len = size();

  if (__len < 1)
    return npos;
  else {
		const_iterator __last = begin() + (min) (__len - 1, __pos) + 1;
		while ( __last != begin() )
		{
			--__last;
			if ( *__last != __c )
				return __last - begin();
		}
		return npos;
/*    const_iterator __last = begin() + (min) (__len - 1, __pos) + 1;
    const_reverse_iterator __rlast = const_reverse_iterator(__last);
    const_reverse_iterator __rresult =
      nstl::find_if(__rlast, rend(),
			 _Neq_char_bound<T>(__c));
    return __rresult != rend() ? (__rresult.base() - 1) - begin() : npos;*/
  }
}


/*template <class T> void  _S_string_copy(const basic_string<T>& __s,
                    T* __buf,
                    size_t __n)
{
  if (__n > 0) {
    __n = (min) (__n - 1, __s.size());
    nstl::copy(__s.begin(), __s.begin() + __n, __buf);
    __buf[__n] = T();
  }
}*/



  






typedef basic_string<char> string;

typedef basic_string<wchar_t> wstring;



template<> struct hash<nstl::string>
{
	unsigned int operator()( const nstl::string &s ) const { return nstl::__stl_hash_string( s.c_str() ); }
};

template<> struct hash<nstl::wstring>
{
	unsigned int operator()( const nstl::wstring &s ) const 
	{ 
		unsigned int r = 0; 
		for ( int k = 0; k < s.length(); ++k )
			r = 5 * r + ((int)s[k]);
		return r; 
	}
};



}
