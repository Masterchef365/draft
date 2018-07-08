/* This should only be included once per config type in the project */

CONFIG_FUNCTION_PREFIX(hash_cache) CONFIG_FUNCTION_PREFIX(create_hash_cache) () {
	CONFIG_FUNCTION_PREFIX(hash_cache) cache;
#define MEMBER(TYPE, NAME, FORMAT) cache.NAME##_hash = str_hash(#NAME);
	MEMBERS();
#undef MEMBER
	return cache;
}

CONFIG_STRUCT_NAME CONFIG_FUNCTION_PREFIX(parse_config) (FILE* file) {
	char key_buf[64];
	char value_buf[64];

	CONFIG_FUNCTION_PREFIX(hash_cache) cache = CONFIG_FUNCTION_PREFIX(create_hash_cache) ();

	CONFIG_STRUCT_NAME conf;

	bzero(&conf, sizeof(conf));
	bzero(key_buf, sizeof(key_buf));
	bzero(value_buf, sizeof(value_buf));

	while (fscanf(file, "%s = %s\n", key_buf, value_buf) > 0) {
		unsigned int key_hash = str_hash (key_buf);

#define MEMBER(TYPE, NAME, FORMAT) if (cache.NAME##_hash == key_hash) sscanf(value_buf, FORMAT, &conf.NAME);
		MEMBERS();
#undef MEMBER

		bzero(key_buf, sizeof(key_buf));
		bzero(value_buf, sizeof(value_buf));
	}
	return conf;
}

void CONFIG_FUNCTION_PREFIX(write_config) (FILE* file, CONFIG_STRUCT_NAME * conf) {
#define MEMBER(TYPE, NAME, FORMAT) fprintf(file, "%s = " FORMAT "\n", #NAME, conf->NAME);
		MEMBERS();
#undef MEMBER
}
