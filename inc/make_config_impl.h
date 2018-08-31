/* This should only be included once per config type in the project */

void CONFIG_FUNCTION_PREFIX(create_hash_cache) (CONFIG_FUNCTION_PREFIX(hash_cache)* cache) {
#define MEMBER(TYPE, NAME, FORMAT) cache->NAME##_hash = str_hash(#NAME);
	MEMBERS();
#undef MEMBER
}

void CONFIG_FUNCTION_PREFIX(parse_config) (FILE* file, CONFIG_STRUCT_NAME* config) {
	char key_buf[64];
	char value_buf[64];

	CONFIG_FUNCTION_PREFIX(hash_cache) cache;
	CONFIG_FUNCTION_PREFIX(create_hash_cache) (&cache);

	bzero(config, sizeof(CONFIG_STRUCT_NAME));
	bzero(key_buf, sizeof(key_buf));
	bzero(value_buf, sizeof(value_buf));

	while (fscanf(file, "%s = %s\n", key_buf, value_buf) > 0) {
		unsigned int key_hash = str_hash (key_buf);

#define MEMBER(TYPE, NAME, FORMAT) if (cache.NAME##_hash == key_hash) sscanf(value_buf, FORMAT, &config->NAME);
		MEMBERS();
#undef MEMBER

		bzero(key_buf, sizeof(key_buf));
		bzero(value_buf, sizeof(value_buf));
	}
}

void CONFIG_FUNCTION_PREFIX(write_config) (FILE* file, CONFIG_STRUCT_NAME *config) {
#define MEMBER(TYPE, NAME, FORMAT) fprintf(file, "%s = " FORMAT "\n", #NAME, config->NAME);
		MEMBERS();
#undef MEMBER
}

int CONFIG_FUNCTION_PREFIX(load_or_write_defaults_from_dir) (char* dir, char* name, CONFIG_STRUCT_NAME* config) {
	char full_dir[1024];
	sprintf(full_dir, "%s/%s", dir, name);
	FILE* config_file_ptr;
	int ret = 0;
	if (!(config_file_ptr = fopen(full_dir, "r"))) {
		inform_log(log_warn, "Could not find %s, creating default.", full_dir);
		config_file_ptr = fopen(full_dir, "w");
		CONFIG_FUNCTION_PREFIX(write_config)(config_file_ptr, config);
		fclose(config_file_ptr);
		return 0;
	} else {
		CONFIG_FUNCTION_PREFIX(parse_config)(config_file_ptr, config);
		fclose(config_file_ptr);
		return 1;
	}
}
