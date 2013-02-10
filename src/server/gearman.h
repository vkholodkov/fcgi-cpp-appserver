
#ifndef _FP_GEARMAN_H_
#define _FP_GEARMAN_H_

#include <string>

class gearman_exception {
public:
	gearman_exception(const std::string &_msg)
		: msg(_msg)
	{
	}

	gearman_exception(gearman_client_st *_client)
		: msg(gearman_client_error(_client))
	{
	}

	gearman_exception(gearman_worker_st *_worker)
		: msg(gearman_worker_error(_worker))
	{
	}

	const char *what() const {
		return msg.c_str();
	}

	std::string msg;
};

#endif
