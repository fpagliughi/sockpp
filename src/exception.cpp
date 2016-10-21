// exception.cpp

#include "sockpp/exception.h"
#include <errno.h>
#include <cstring>

using namespace std;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

sys_error::sys_error() : sys_error(errno)
{
}

sys_error::sys_error(int err) : runtime_error(strerror(err)), errno_(err)
{
}


/////////////////////////////////////////////////////////////////////////////
// end namespace 'sockpp'
}

