/** @file
@brief The precompiled header.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>

Defines #HIVE_PCH macro if precompiled headers are enabled.

@see @ref page_hive_pch
*/
#if 1 // enable/disable PCH

// boost
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/function.hpp>
#include <boost/cstdint.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

// STL
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <limits>
#include <deque>
#include <queue>
#include <list>
#include <map>
#include <set>

// libc
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/// @brief The precompiled headers indicator.
#define HIVE_PCH 1
#endif // PCH


///////////////////////////////////////////////////////////////////////////////
/** @page page_hive_pch Precompiled headers

The <hive/pch.hpp> file contains commonly used boost, STL and libc headers.

To use this file as precompiled header you have to include it into every
source file.

For example the *MyDevice.cpp* file:
~~~{.cpp}
#include <hive/pch.hpp>
#include "MyDevice.hpp"

//// MyDevice implementation...

MyDevice::MyDevice()
{
    ...
}

...
~~~

and the *MyDevice.hpp* header file:
~~~{.cpp}
#if !defined(HIVE_PCH)
#   include <string>
#   include <vector>
#endif

//// MyDevice
class MyDevice
{
public:
    MyDevice();

    ...
};
~~~

Anyway the header file should include all necessary headers and the
`#%if !defined(HIVE_PCH)` section is used when precompiled headers
are disabled.

There is also possible to compile header file using `gcc` compiler.
It produces corresponding `*.gch` file.
*/
