#include "common.hxx"
#include "filter.hxx"

namespace LIBCLI
{

using namespace std;

int
filter_t::exec
(
	CLI& Instance,
	char *cmd
)
{
	DEBUG d (__FUNCTION__,__FILE__,__LINE__);
	return (CALL_MEMBER_FN (Instance, this->filter)(cmd, this->data));
}

int
filter_t::exec
(
	CLI& Instance,
	char *cmd,
	void *data
)
{
	DEBUG d (__FUNCTION__,__FILE__,__LINE__);
	return (CALL_MEMBER_FN (Instance, this->filter)(cmd, data));
}

}; // namespace LIBCLI

