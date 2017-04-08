#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "php.h"
#include "ext/standard/info.h"
#include "phpwkhtmltox.h"
#include "wkhtmltox/pdf.h"
#include "wkhtmltox/image.h"

ZEND_DECLARE_MODULE_GLOBALS(phpwkhtmltox)

static zend_function_entry phpwkhtmltox_functions[] = {
  PHP_FE(wkhtmltox_convert, NULL)
  {
NULL, NULL, NULL
}
};

static void php_phpwkhtmltox_init_globals(zend_phpwkhtmltox_globals *phpwkhtmltox_globals)
{
	phpwkhtmltox_globals->wkhtmltoimage_initialized = 0;
	phpwkhtmltox_globals->wkhtmltopdf_initialized = 0;
}

PHP_MINIT_FUNCTION(phpwkhtmltox)
{
	ZEND_INIT_MODULE_GLOBALS(phpwkhtmltox, php_phpwkhtmltox_init_globals, NULL);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(phpwkhtmltox)
{
	if (PHPWKHTMLTOX_G(wkhtmltoimage_initialized)) {
		wkhtmltoimage_deinit();
	}

	if (PHPWKHTMLTOX_G(wkhtmltopdf_initialized)) {
		wkhtmltopdf_deinit();
	}

	return SUCCESS;
}

PHP_RINIT_FUNCTION(phpwkhtmltox)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(phpwkhtmltox)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(phpwkhtmltox)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "phpwkhtmltox", "enabled");
	php_info_print_table_row(2, "version", wkhtmltopdf_version());
	php_info_print_table_end();
}

zend_module_entry phpwkhtmltox_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
  STANDARD_MODULE_HEADER,
#endif
  PHP_PHPWKHTMLTOX_EXTNAME,
  phpwkhtmltox_functions,
  PHP_MINIT(phpwkhtmltox),
  PHP_MSHUTDOWN(phpwkhtmltox),
  PHP_RINIT(phpwkhtmltox),
  PHP_RSHUTDOWN(phpwkhtmltox),
  PHP_MINFO(phpwkhtmltox),
#if ZEND_MODULE_API_NO >= 20010901
  PHP_PHPWKHTMLTOX_VERSION,
#endif
  STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PHPWKHTMLTOX
ZEND_GET_MODULE(phpwkhtmltox)
#endif

/**
 * bool wkhtmltox_convert(format, array global_settings, 2d array settings)
 */
	PHP_FUNCTION(wkhtmltox_convert)
{
	// used for iterating over the object params
	zval **data;
	HashTable *params_hash;
	HashPosition pointer;
	int params_count;

	// temporarily holds return value
	int ret;

	// receives the format parameter
	char *format = NULL;
	size_t format_len;

	// receives the global and object parameters
	zval *global_params;
	zval *object_params;

	// php7 new type
	size_t name_len;
	zend_string *strg;


	// parse out parameters passed
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa|a",
		&format, &format_len, &global_params, &object_params) == FAILURE) {
		RETURN_NULL();
	}

	strg = strpprintf(0, "%s", format);
	// RETURN_STR(strg);

	if (zend_string_equals_literal(strg, "image")) {
		// RETURN_STR(strg);
		if (!PHPWKHTMLTOX_G(wkhtmltoimage_initialized)) {
			PHPWKHTMLTOX_G(wkhtmltoimage_initialized) = wkhtmltoimage_init(0);
		}

		wkhtmltoimage_global_settings *global_settings = wkhtmltoimage_create_global_settings();

		wkhtmltox_set_params((void *)global_settings, (fp)wkhtmltoimage_set_global_setting, global_params);

		wkhtmltoimage_converter *c = wkhtmltoimage_create_converter(global_settings, NULL);
		ret = wkhtmltoimage_convert(c);
		wkhtmltoimage_destroy_converter(c);

	}
	else if (zend_string_equals_literal(strg, "pdf")) {
		if (!PHPWKHTMLTOX_G(wkhtmltopdf_initialized)) {
			PHPWKHTMLTOX_G(wkhtmltopdf_initialized) = wkhtmltopdf_init(0);
		}

		wkhtmltopdf_global_settings *global_settings = wkhtmltopdf_create_global_settings();
		wkhtmltox_set_params((void *)global_settings, (fp)wkhtmltopdf_set_global_setting, global_params);

		wkhtmltopdf_converter *c = wkhtmltopdf_create_converter(global_settings);

		params_hash = Z_ARRVAL_P(object_params);
		params_count = zend_hash_num_elements(params_hash);
		for (zend_hash_internal_pointer_reset_ex(params_hash, &pointer);
			zend_hash_has_more_elements_ex(params_hash, &pointer) == SUCCESS;
			zend_hash_move_forward_ex(params_hash, &pointer)) {
			zval * data = zend_hash_get_current_data_ex(params_hash, &pointer);
			zval temp = *data;
			zval_copy_ctor(&temp);
			/* if (Z_TYPE_P(data) == IS_STRING) { */
			/*           PHPWRITE(Z_STRVAL_P(data), Z_STRLEN_P(data)); */
			/*           php_printf(""); */
			/* } */
			if (Z_TYPE(temp) == IS_ARRAY) {
				wkhtmltopdf_object_settings *object_settings = wkhtmltopdf_create_object_settings();
				wkhtmltox_set_params((void *)object_settings, (fp)wkhtmltopdf_set_object_setting, &temp);
				wkhtmltopdf_add_object(c, object_settings, NULL);
			}

			zval_dtor(&temp);
		}

		ret = wkhtmltopdf_convert(c);
		wkhtmltopdf_destroy_converter(c);
	}

	RETVAL_BOOL(ret);
}

void wkhtmltox_set_params(void *settings, fp set_function, zval *params)
{
	zval *data;
	HashTable *params_hash;
	HashPosition pointer;
	int params_count;

	char *key;
	int key_len;
	long index;
	zend_string *str_index;
	zend_ulong num_index;

	params_hash = Z_ARRVAL_P(params);
	params_count = zend_hash_num_elements(params_hash);
	for (zend_hash_internal_pointer_reset_ex(params_hash, &pointer);
		zend_hash_has_more_elements_ex(params_hash, &pointer) == SUCCESS;
		zend_hash_move_forward_ex(params_hash, &pointer)) {
		data = zend_hash_get_current_data_ex(params_hash, &pointer);
		zval temp = *data;
		zval_copy_ctor(&temp);

		if (zend_hash_get_current_key_ex(params_hash, &str_index, &num_index, &pointer) == HASH_KEY_IS_STRING) {

			switch (Z_TYPE(temp)) {
			case IS_TRUE:
				set_function(settings, str_index->val, "true");
				break;
			case IS_FALSE:
				set_function(settings, str_index->val, "false");
				break;
			case IS_STRING:
				set_function(settings, str_index->val, Z_STRVAL(temp));
				break;
			default:
				convert_to_string(&temp);

			}

			/*
			  convert_to_string(&temp);
			  php_printf("%s => %s\n", key, Z_STRVAL(temp));
			*/
		}

		zval_dtor(&temp);
	}
}
