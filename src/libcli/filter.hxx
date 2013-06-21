#ifndef CLI_FILTER_H
#define CLI_FILTER_H

namespace LIBCLI
{

struct filter_cmds_t
{
	const char *cmd;
	const char *help;
};

class CLI;
typedef int (CLI::*filter_callback_func) (char *cmd, void *data);

class filter_t
{
public:
	filter_callback_func filter;
	void *data;
	filter_t *next;
	int exec (CLI& Instance, char *cmd);
	int exec (CLI& Instance, char *cmd, void *data);
};


}; // namespace LIBCLI

#endif
