#include <atmel_start.h>

#include "model.h"

Model &Model::instance() {
	static Model model;
	if (!model.initialized) {
		model.initialized = true;
		model.init();
	}
	return model;
}

void Model::init() {
}
