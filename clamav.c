/*
 *  Copyright (C) 2012 Argos <argos66@gmail.com>
 *  Copyright (C) 2005 Geffrey Velasquez Torres <geffrey@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_clamav.h"
#include <string.h>
#include <stdio.h>
#include <clamav.h>

ZEND_DECLARE_MODULE_GLOBALS(clamav)

/* True global resources - no need for thread safety here */
//static int le_clamav;

/* {{{ arginfo */
#if (PHP_MAJOR_VERSION >= 6 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3))
# define PHP_CLAMAV_ARGINFO
#else
# define PHP_CLAMAV_ARGINFO static
#endif

PHP_CLAMAV_ARGINFO
ZEND_BEGIN_ARG_INFO(second_args_force_ref, 0)
    ZEND_ARG_PASS_INFO(0)
    ZEND_ARG_PASS_INFO(1)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ clamav_functions[]
 *
 * Every user visible function must have an entry in clamav_functions[].
 */
zend_function_entry clamav_functions[] = {
    PHP_FE(cl_info, NULL)
    PHP_FE(cl_scanfile, second_args_force_ref)
    PHP_FE(cl_engine, NULL)
    PHP_FE(cl_pretcode, NULL)
    PHP_FE(cl_version, NULL)
    PHP_FE(cl_debug, NULL)
    {NULL, NULL, NULL}       /* Must be the last line in clamav_functions[] */
};
/* }}} */

/* {{{ clamav_module_entry
 */
zend_module_entry clamav_module_entry = {
    STANDARD_MODULE_HEADER,
    "clamav",
    clamav_functions,
    PHP_MINIT(clamav),
    PHP_MSHUTDOWN(clamav),
    PHP_RINIT(clamav),        
    PHP_RSHUTDOWN(clamav),
    PHP_MINFO(clamav),
    PHP_CLAMAV_VERSION,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_CLAMAV
ZEND_GET_MODULE(clamav)
#endif

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()

    STD_PHP_INI_ENTRY("clamav.dbpath", "/var/lib/clamav", PHP_INI_ALL, 
                      OnUpdateString, dbpath, zend_clamav_globals, clamav_globals)

    STD_PHP_INI_ENTRY("clamav.maxreclevel", "16", PHP_INI_ALL, OnUpdateLong, 
                       maxreclevel, zend_clamav_globals, clamav_globals)

    STD_PHP_INI_ENTRY("clamav.maxfiles", "10000", PHP_INI_ALL, OnUpdateLong, 
                      maxfiles, zend_clamav_globals, clamav_globals)

    STD_PHP_INI_ENTRY("clamav.maxfilesize", "26214400", PHP_INI_ALL, OnUpdateLong, 
                      maxfilesize, zend_clamav_globals, clamav_globals)

    STD_PHP_INI_ENTRY("clamav.maxscansize", "104857600", PHP_INI_ALL, OnUpdateLong, 
                      maxscansize, zend_clamav_globals, clamav_globals)

    STD_PHP_INI_ENTRY("clamav.keeptmp", "0", PHP_INI_ALL, OnUpdateLong, 
                      keeptmp, zend_clamav_globals, clamav_globals)

    STD_PHP_INI_ENTRY("clamav.load_db_on_startup", "0", PHP_INI_ALL, OnUpdateLong, 
                      load_db_on_startup, zend_clamav_globals, clamav_globals)

    STD_PHP_INI_ENTRY("clamav.tmpdir", "/tmp", PHP_INI_ALL, OnUpdateString, 
                      tmpdir, zend_clamav_globals, clamav_globals)

PHP_INI_END()
/* }}} */

static int clamav_load_database()
{
    int ret    = 0;    /* return value */
    int reload = 0;

    /* libclamav initialization */
    if (CLAMAV_G(cl_initcalled) == 0){
		if((ret = cl_init(CL_INIT_DEFAULT)) != CL_SUCCESS) {
			php_error(E_WARNING, "cl_init: failed : error code %i (%s)\n", ret, cl_strerror(ret));
			return FAILURE;
		} else {
			CLAMAV_G(cl_initcalled) = 1;
		}
	}

	/* database reload */
    if (CLAMAV_G(dbstat.dir) != NULL && cl_statchkdir(&CLAMAV_G(dbstat)) == 1) {
		reload = 1;
        cl_statfree(&CLAMAV_G(dbstat));
	}

	/* load engine */
    if (!(CLAMAV_G(dbengine) = cl_engine_new())){
        php_error(E_WARNING, "Can’t create new engine\n");
        return FAILURE;
    }

	if (CLAMAV_G(dbpath) == NULL) CLAMAV_G(dbpath) = (char *) cl_retdbdir();

    /* database loading */
    if ((ret = cl_load(CLAMAV_G(dbpath), CLAMAV_G(dbengine), &CLAMAV_G(sig_num), CL_DB_STDOPT)) != CL_SUCCESS) {
        php_error(E_WARNING, "cl_load: failed : error code %i (%s)\n", ret, cl_strerror(ret));
        return FAILURE;
    }

	/* compile signature database */
    if ((ret = cl_engine_compile(CLAMAV_G(dbengine))) != CL_SUCCESS) {
        php_error(E_WARNING, "cl_engine_compile : error code %i (%s\n", ret, cl_strerror(ret));
        return FAILURE;
    }

    /* allocate cl_stat */
    if (!reload) memset(&CLAMAV_G(dbstat), 0, sizeof(struct cl_stat));

	/* database stats */
    if ((ret = cl_statinidir(CLAMAV_G(dbpath), &CLAMAV_G(dbstat))) != CL_SUCCESS) {
        php_error(E_WARNING, "cl_statinidir : error code %i (%s\n", ret, cl_strerror(ret));
        return FAILURE;
	}

    /* set engine parameters */
    cl_engine_set_num(CLAMAV_G(dbengine), CL_ENGINE_MAX_FILES, CLAMAV_G(maxfiles));
    cl_engine_set_num(CLAMAV_G(dbengine), CL_ENGINE_MAX_FILESIZE, CLAMAV_G(maxfilesize));
    cl_engine_set_num(CLAMAV_G(dbengine), CL_ENGINE_MAX_SCANSIZE, CLAMAV_G(maxscansize));
    cl_engine_set_num(CLAMAV_G(dbengine), CL_ENGINE_MAX_RECURSION, CLAMAV_G(maxreclevel));
    cl_engine_set_num(CLAMAV_G(dbengine), CL_ENGINE_KEEPTMP, CLAMAV_G(keeptmp));
    cl_engine_set_str(CLAMAV_G(dbengine), CL_ENGINE_TMPDIR, CLAMAV_G(tmpdir));

    return ret;
}

/* {{{ php_clamav_init_globals
 */
static void php_clamav_init_globals(zend_clamav_globals *clamav_globals)
{
    clamav_globals->dbpath        = NULL;
    clamav_globals->maxreclevel   = 0;
    clamav_globals->maxfiles      = 0;
    clamav_globals->maxfilesize   = 0;
    clamav_globals->maxscansize   = 0;
    clamav_globals->keeptmp       = 0;
    clamav_globals->load_db_on_startup = 0;
    clamav_globals->tmpdir        = NULL;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(clamav)
{
	ZEND_INIT_MODULE_GLOBALS(clamav, php_clamav_init_globals, NULL);
    REGISTER_INI_ENTRIES();
	CLAMAV_G(cl_initcalled) = 0;

    /*  Register ClamAV scan options, they're also availible inside
     *  php scripts with the same name and value.
     */
    REGISTER_LONG_CONSTANT("CL_SCAN_RAW", CL_SCAN_RAW, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_ARCHIVE", CL_SCAN_ARCHIVE, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_MAIL", CL_SCAN_MAIL, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_OLE2", CL_SCAN_OLE2, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_BLOCKENCRYPTED", CL_SCAN_BLOCKENCRYPTED, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_HTML", CL_SCAN_HTML, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_PE", CL_SCAN_PE, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_BLOCKBROKEN", CL_SCAN_BLOCKBROKEN, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_MAILURL", CL_SCAN_MAILURL, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_BLOCKMAX", CL_SCAN_BLOCKMAX, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_ALGORITHMIC", CL_SCAN_ALGORITHMIC, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_ELF", CL_SCAN_ELF, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_PDF", CL_SCAN_PDF, 
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_SCAN_STDOPT", CL_SCAN_STDOPT, 
                            CONST_CS | CONST_PERSISTENT);

    /*  Register ClamAV return codes, they're also available
     *  inside php scripts with the same name and value.
     */
    REGISTER_LONG_CONSTANT("CL_CLEAN", CL_CLEAN,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_VIRUS", CL_VIRUS,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_ENULLARG", CL_ENULLARG,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EARG", CL_EARG,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EMALFDB", CL_EMALFDB,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_ECVD", CL_ECVD,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EVERIFY", CL_EVERIFY,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EUNPACK", CL_EUNPACK,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EOPEN", CL_EOPEN,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_ECREAT", CL_ECREAT,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EUNLINK", CL_EUNLINK,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_ESTAT", CL_ESTAT,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EREAD", CL_EREAD,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_ESEEK", CL_ESEEK,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EWRITE", CL_EWRITE,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EDUP", CL_EDUP,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EACCES", CL_EACCES,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_ETMPFILE", CL_ETMPFILE,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_ETMPDIR", CL_ETMPDIR,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EMAP", CL_EMAP,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EMEM", CL_EMEM,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_ETIMEOUT", CL_ETIMEOUT,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EMAXREC", CL_EMAXREC,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EMAXSIZE", CL_EMAXSIZE,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EMAXFILES", CL_EMAXFILES,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_EFORMAT", CL_EFORMAT,
                            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CL_ELAST_ERROR", CL_ELAST_ERROR,
                            CONST_CS | CONST_PERSISTENT);

	if (CLAMAV_G(load_db_on_startup) == 1) {
		int ret = 0;

		if ((ret = clamav_load_database()) != CL_SUCCESS) {
			php_error(E_WARNING, "Load database during PHP_MINIT_FUNCTION failed : error code %i (%s)\n", ret, cl_strerror(ret));
			return FAILURE;
		}
	}

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(clamav)
{
    UNREGISTER_INI_ENTRIES();
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(clamav)
{
	if (CLAMAV_G(load_db_on_startup) == 1) {
		int ret = 0;

		if ((ret = clamav_load_database()) != CL_SUCCESS) {
			php_error(E_WARNING, "Load database during PHP_RINIT_FUNCTION failed : error code %i (%s)\n", ret, cl_strerror(ret));
			return FAILURE;
		}
	}

    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(clamav)
{
    UNREGISTER_INI_ENTRIES();
    if (CLAMAV_G(load_db_on_startup) == 0) {
		if (CLAMAV_G(dbengine)  != NULL) cl_engine_free(CLAMAV_G(dbengine));
		CLAMAV_G(dbengine) = 0;
	}
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(clamav)
{
    if (CLAMAV_G(load_db_on_startup) == 0) {
		int ret = 0;

		if ((ret = clamav_load_database()) != CL_SUCCESS) {
			php_error(E_WARNING, "Load database during PHP_MINFO_FUNCTION failed : error code : %i (%s)\n", ret, cl_strerror(ret));
			return;
		}
	}

    php_info_print_table_start();
    php_info_print_table_row(2, "Clamav support", "enabled");
    php_info_print_table_row(2, "php-clamav version", PHP_CLAMAV_VERSION);
    php_info_print_table_row(2, "libclamav version", cl_retver());
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ proto void cl_info()
    Prints ClamAV informations */
PHP_FUNCTION(cl_info)
{
    if (CLAMAV_G(load_db_on_startup) == 0) {
		int ret = 0;

		if ((ret = clamav_load_database()) != CL_SUCCESS) {
			php_error(E_WARNING, "Load database during cl_info failed : error code : %i (%s)\n", ret, cl_strerror(ret));
			RETURN_FALSE;
		}
	}

	php_printf("ClamAV version %s with %d virus signatures loaded", cl_retver(), CLAMAV_G(sig_num));
}
/* }}} */

/* {{{ proto void cl_version()
    Prints only ClamAV version */
PHP_FUNCTION(cl_version)
{
    if (CLAMAV_G(load_db_on_startup) == 0) {
		int   ret = 0;

		if ((ret = clamav_load_database()) != CL_SUCCESS) {
			php_error(E_WARNING, "Load database during cl_version failed : error code : %i (%s)\n", ret, cl_strerror(ret));
			RETURN_FALSE;
		}
	}

    RETURN_STRING((char *) cl_retver(), 1);
}
/* }}} */

/* {{{ proto int cl_scanfile(string filename) 
 * Scans the contents of a file, given a filename, returns the virus name
 * (if it was found) and return ClamAV cl_scanfile result code */
PHP_FUNCTION(cl_scanfile)
{
	static int db_loaded   = 0;
	const int   NUM_ARGS   = 2;
	const char *virname	   = "";
    zval       *filename;          /* file to be scanned */
    zval       *virusname;         /* for return virus name if found */
	int         ret        = 0;    /* clamav functions return value */

	MAKE_STD_ZVAL(filename);
	MAKE_STD_ZVAL(virusname);

	if (ZEND_NUM_ARGS() != NUM_ARGS) {
		WRONG_PARAM_COUNT;
		RETURN_FALSE;
	}

	if (zend_parse_parameters(NUM_ARGS TSRMLS_CC, "zz", &filename, &virusname) != SUCCESS) {
        WRONG_PARAM_COUNT;
		RETURN_FALSE;
	}

    if (CLAMAV_G(load_db_on_startup) == 0 && (!db_loaded)) {
		if ((ret = clamav_load_database()) != CL_SUCCESS) {
			php_error(E_WARNING, "Load database during cl_scanfile failed : error : %i (%s)\n", ret, cl_strerror(ret));
			RETURN_FALSE;
		} else {
			db_loaded = 1;
		}
	}

	convert_to_string_ex(&filename); // parameter conversion
	zval_dtor(virusname); // clean up old values first

	/* executing the ClamAV virus checking function */
    ret = cl_scanfile(Z_STRVAL_P(filename), &virname, 0, CLAMAV_G(dbengine), CL_SCAN_STDOPT);

	/* copy the value of the cl_scanfile virusname if a virus was found */
	if (ret == CL_VIRUS) {
        ZVAL_STRING(virusname, virname, 1);
    } else if (ret != CL_CLEAN) {
        php_error(E_WARNING,"Error: %i (%s)", ret, cl_strerror(ret));
	}

    RETURN_LONG(ret);
}
/* }}} */

/* {{{ proto void cl_engine()
    Set the ClamAV parameters for scanning */ 
PHP_FUNCTION(cl_engine)
{
    const int NUM_ARGS    = 5;
    long long maxfiles    = 0;
    long long maxfilesize = 0;
    long long maxscansize = 0;
    long      maxreclevel = 0;
    long      keeptmp     = 0;

    /* argument checking */
    if (ZEND_NUM_ARGS() != NUM_ARGS) {
        WRONG_PARAM_COUNT;
        RETURN_FALSE;
    }

    /* argument parsing */
    if (zend_parse_parameters(NUM_ARGS TSRMLS_CC, "lllll", 
                              &maxfiles, &maxfilesize, &maxscansize,
                              &maxreclevel, &keeptmp ) != SUCCESS) {
        RETURN_FALSE;
    }

    /* set engine parameters */
    cl_engine_set_num(CLAMAV_G(dbengine), CL_ENGINE_MAX_FILES, maxfiles);
    cl_engine_set_num(CLAMAV_G(dbengine), CL_ENGINE_MAX_FILESIZE, maxfilesize);
    cl_engine_set_num(CLAMAV_G(dbengine), CL_ENGINE_MAX_SCANSIZE, maxscansize);
    cl_engine_set_num(CLAMAV_G(dbengine), CL_ENGINE_MAX_RECURSION, maxreclevel);
    cl_engine_set_num(CLAMAV_G(dbengine), CL_ENGINE_KEEPTMP, keeptmp);

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string cl_pretcode(int retcode)
    Translates the ClamAV return code */
PHP_FUNCTION(cl_pretcode)
{
    const int NUM_ARGS    = 1;
    long      retcode     = 0;

    if (ZEND_NUM_ARGS() != NUM_ARGS) {
        WRONG_PARAM_COUNT;
        RETURN_FALSE;
    }

    /* parameters parsing */
    if (zend_parse_parameters(NUM_ARGS TSRMLS_CC, "l", &retcode) != SUCCESS) {
        WRONG_PARAM_COUNT;
        RETURN_FALSE;
    }

    switch (retcode) {
        /* libclamav specific errors*/
        case CL_CLEAN:
            RETURN_STRING("virus not found", 1);
            break;
        case CL_VIRUS:
            RETURN_STRING("virus found", 1);
            break;
        case CL_ENULLARG:
            RETURN_STRING("null argument error", 1);
            break;
        case CL_EARG:
            RETURN_STRING("argument error", 1);
            break;
        case CL_EMALFDB:
            RETURN_STRING("malformed database", 1);
            break;
        case CL_ECVD:
            RETURN_STRING("CVD error", 1);
            break;
        case CL_EVERIFY:
            RETURN_STRING("verification error", 1);
            break;
        case CL_EUNPACK:
            RETURN_STRING("uncompression error", 1);
            break;

        /* I/O and memory errors */
        case CL_EOPEN:
            RETURN_STRING("CL_EOPEN error", 1);
            break;
        case CL_ECREAT:
            RETURN_STRING("CL_ECREAT error", 1);
            break;
        case CL_EUNLINK:
            RETURN_STRING("CL_EUNLINK error", 1);
            break;
        case CL_ESTAT:
            RETURN_STRING("CL_ESTAT error", 1);
            break;
        case CL_EREAD:
            RETURN_STRING("CL_EREAD error", 1);
            break;
        case CL_ESEEK:
            RETURN_STRING("CL_ESEEK error", 1);
            break;
        case CL_EWRITE:
            RETURN_STRING("CL_EWRITE error", 1);
            break;
        case CL_EDUP:
            RETURN_STRING("CL_EDUP error", 1);
            break;
        case CL_EACCES:
            RETURN_STRING("CL_EACCES error", 1);
            break;
        case CL_ETMPFILE:
            RETURN_STRING("CL_ETMPFILE error", 1);
            break;
        case CL_ETMPDIR:
            RETURN_STRING("CL_ETMPDIR error", 1);
            break;
        case CL_EMAP:
            RETURN_STRING("CL_EMAP error", 1);
            break;
        case CL_EMEM:
            RETURN_STRING("CL_EMEM error", 1);
            break;
        case CL_ETIMEOUT:
            RETURN_STRING("CL_ETIMEOUT error", 1);
            break;

        /* internal (not reported outside libclamav) */
        case CL_BREAK:
            RETURN_STRING("CL_BREAK error", 1);
            break;
        case CL_EMAXREC:
            RETURN_STRING("recursion level limit exceeded", 1);
            break;
        case CL_EMAXSIZE:
            RETURN_STRING("maximum file size limit exceeded", 1);
            break;
        case CL_EMAXFILES:
            RETURN_STRING("maximum files limit exceeded", 1);
            break;
        case CL_EFORMAT:
            RETURN_STRING("bad format or broken file", 1);
            break;
        case CL_ELAST_ERROR:
            RETURN_STRING("CL_ELAST_ERROR error", 1);
            break;

        default :
            RETURN_STRING("unknow return code", 1);
            break;
    }

    RETURN_STRING("No matching error", 1);
}
/* }}} */

/* {{{ proto string cl_debug()
    Turn on ClamAV debug mode */
PHP_FUNCTION(cl_debug)
{
    cl_debug();
}
/* }}} */
