echo '#define CURV_VERSION "'`git describe --tags --always --dirty`'"' >,v
if cmp -s ,v libcurv/version.h; then rm ,v; else mv ,v libcurv/version.h; fi
