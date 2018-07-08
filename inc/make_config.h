/* 
 * Duncan's configuration file parser generator.
 * Usage example: 
 *  
 * #define CONFIG_STRUCT_NAME MotorController
 * #define CONFIG_FUNCTION_PREFIX(NAME) motor_##NAME
 * #define MEMBERS() \
 * 	MEMBER(float, pgain, "%f") \
 * 	MEMBER(float, fgain, "%f") \
 * 
 * #include <make_config.h>
 * #undef MEMBERS
 *
 * ...
 *
 * MotorController conf = motor_parse_config(config_file);
 * printf("pgain: %f\n", conf.pgain);
 * printf("fgain: %f\n", conf.fgain);
 * freopen(argv[1], "w", config_file);
 * motor_write_config(config_file, &conf);
 */

#ifndef MAKE_CONFIG_H
#define MAKE_CONFIG_H

#include <string.h>
#include <errno.h>

/* Convert a string into a unique hash */
unsigned int str_hash(char* str) {
	unsigned int hash = 7890;
	int c;
	while ((c = *str++)) hash += (hash << 5) + c;
	return hash;
}
#endif

/* Create the members of the configuration storage struct 
 * (named CONFIG_STRUCT_NAME) using the MEMBERS() macro */
#define MEMBER(TYPE, NAME, FORMAT) TYPE NAME;
typedef struct CONFIG_STRUCT_NAME {
	MEMBERS();
} CONFIG_STRUCT_NAME;
#undef MEMBER

/* Create a struct to store a hash table linking
 * the names of variables to their respective hashes */
#define MEMBER(TYPE, NAME, FORMAT) unsigned int NAME##_hash;
typedef struct CONFIG_FUNCTION_PREFIX(hash_cache) {
	MEMBERS();
} CONFIG_FUNCTION_PREFIX(hash_cache);
#undef MEMBER

/* Create the function <prefix>_create_hash_cache(),
 * instantiates an isntance of the <prefix>_hash_cache struct 
 * and populates it with the hashes of all of the variable names */
CONFIG_FUNCTION_PREFIX(hash_cache) CONFIG_FUNCTION_PREFIX(create_hash_cache) () {
	CONFIG_FUNCTION_PREFIX(hash_cache) cache;
#define MEMBER(TYPE, NAME, FORMAT) cache.NAME##_hash = str_hash(#NAME);
	MEMBERS();
#undef MEMBER
	return cache;
}

/* Create the function <prefix>_parse_config() which 
 * creates an instance of the Config struct (named CONFIG_STRUCT_NAME)
 * given a file pointer to parse the config from. */
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

/* Create the function <prefix>_write_config() which will create
 * a parseable and human readable config file from an existing config
 * structure. */
void CONFIG_FUNCTION_PREFIX(write_config) (FILE* file, CONFIG_STRUCT_NAME * conf) {
#define MEMBER(TYPE, NAME, FORMAT) fprintf(file, "%s = " FORMAT "\n", #NAME, conf->NAME);
		MEMBERS();
#undef MEMBER
}
