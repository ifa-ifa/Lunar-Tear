#include <atomic>

enum INFOPATCH_STATUS {
	INFOPATCH_INCOMPLETE,
	INFOPATCH_READY,
	INFOPATCH_ERROR
};

extern std::atomic<INFOPATCH_STATUS> infopatch_status;