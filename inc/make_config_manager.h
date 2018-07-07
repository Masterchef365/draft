/* An addition to make_config.h that allows even easier usage 
 * in cases where the config file is used as a persistence mechanism */

#include <stdlib.h>

/* How long internal file name buffers are */
#define FILENAME_LENGTH 256

typedef struct CONFIG_MANAGER_STRUCT_NAME {
	char location[FILENAME_LENGTH];
	FILE* file_pointer;
	CONFIG_STRUCT_NAME config;
} CONFIG_MANAGER_STRUCT_NAME;

CONFIG_MANAGER_STRUCT_NAME CONFIG_FUNCTION_PREFIX(config_manager_load_from_directory) (char* dir, char* name) {
	CONFIG_MANAGER_STRUCT_NAME manager;
	snprintf(manager.location, FILENAME_LENGTH, "%s/%s", dir, name);

	if (manager.file_pointer = fopen(manager.location, "r")) {
		manager.config = CONFIG_FUNCTION_PREFIX(parse_config) (manager.file_pointer);
	} else {
		fprintf(stderr, "\e[91mError reading %s; %s \e[m\n", name, strerror(errno));
	}
	return manager;
}

void CONFIG_FUNCTION_PREFIX(config_manager_close_and_rewrite) (CONFIG_MANAGER_STRUCT_NAME* manager) {
	if (manager->file_pointer) {
		freopen(manager->location, "w", manager->file_pointer);
		CONFIG_FUNCTION_PREFIX(write_config) (manager->file_pointer, &manager->config);
		fclose(manager->file_pointer);
	}
}

#undef APPEND_MANAGER
