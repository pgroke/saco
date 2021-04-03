#include "_poison_std_types_in_global_namespace.h"

// make sure our headers don't reference standard types like std::size_t in the global namespace
#include <saco/saco.h>
#include <saco/shared_ptr.h>

int main() {
	// avoid empty object file warning
}
