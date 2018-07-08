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

#include <stdio.h>
#include <string.h>
#include <util.h>
#include <errno.h>

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
extern CONFIG_FUNCTION_PREFIX(hash_cache) CONFIG_FUNCTION_PREFIX(create_hash_cache) ();

/* Create the function <prefix>_parse_config() which 
 * creates an instance of the Config struct (named CONFIG_STRUCT_NAME)
 * given a file pointer to parse the config from. */
extern CONFIG_STRUCT_NAME CONFIG_FUNCTION_PREFIX(parse_config) (FILE* file);

/* Create the function <prefix>_write_config() which will create
 * a parseable and human readable config file from an existing config
 * structure. */
extern void CONFIG_FUNCTION_PREFIX(write_config) (FILE* file, CONFIG_STRUCT_NAME * conf);

/* Should only be included with this macro once per project in a 
 * *_config.c file, creates the actual implementations */ 
#ifdef MAKE_IMPL
#include <make_config_impl.h>
#endif
