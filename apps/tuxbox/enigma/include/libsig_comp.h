#ifndef __LIBSIG_COMP_H
#define __LIBSIG_COMP_H

#include <sigc++/sigc++.h>
#include <sigc++/bind.h>

#ifdef SIGC_CXX_NAMESPACES
using namespace sigc;
#endif

#define CONNECT(SENDER, EMPFAENGER) SENDER.connect( mem_fun(*this, &EMPFAENGER))
// use this Makro to connect with a method
// void bla::foo(int x);
// to an
// sigc::signal<void, int> testSig;
//
// CONNECT(testSig, bla::foo);
// signal and method (slot)(mem_fun) must have the same signature

#define CONNECT_1_0(SENDER, EMPFAENGER, PARAM) SENDER.connect( bind( mem_fun(*this, &EMPFAENGER) ,PARAM ) )
// use this for connect with a method
// void bla::foo(int);
// to an
// sigc::signal<void> testSig;
// CONNECT_1_0(testSig, bla:foo, 0);
// here the signal has no parameter, but the slot have an int
// the last parameter of the CONNECT_1_0 makro is the value that given to the paramater of the Slot method

#define CONNECT_2_0(SENDER, EMPFAENGER, PARAM1, PARAM2) SENDER.connect( bind( mem_fun(*this, &EMPFAENGER) ,PARAM1, PARAM2 ) )

#define CONNECT_2_1(SENDER, EMPFAENGER, PARAM) SENDER.connect( bind( mem_fun(*this, &EMPFAENGER) ,PARAM ) )

#endif // __LIBSIG_COMP_H
