/*
 *  Copyright (C) 2011 Argos <argos66@gmail.com> 
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

#ifndef PHP_CLAMAV_H
#define PHP_CLAMAV_H

#include <clamav.h>

extern zend_module_entry clamav_module_entry;
#define phpext_clamav_ptr &clamav_module_entry

#ifdef PHP_WIN32
#    define PHP_CLAMAV_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#    define PHP_CLAMAV_API __attribute__ ((visibility("default")))
#else
#    define PHP_CLAMAV_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define PHP_CLAMAV_VERSION "0.15.8"

PHP_MINIT_FUNCTION(clamav);
PHP_MSHUTDOWN_FUNCTION(clamav);
PHP_RINIT_FUNCTION(clamav);
PHP_RSHUTDOWN_FUNCTION(clamav);
PHP_MINFO_FUNCTION(clamav);

/* functions */
PHP_FUNCTION(cl_info);
PHP_FUNCTION(cl_scanfile);
PHP_FUNCTION(cl_engine);
PHP_FUNCTION(cl_pretcode);
PHP_FUNCTION(cl_version);
PHP_FUNCTION(cl_debug);

static int clamav_load_database();

ZEND_BEGIN_MODULE_GLOBALS(clamav)
    char     *dbpath;      /* virus database path */ 
    long long maxfiles;    /* maximal number of file to be scanned within an archive */
    long long maxfilesize; /* files in an archive larger than this limit will not be scanned */
    long long maxscansize; /* archive larger than this limit will not be scanned */
    long      maxreclevel; /* maximal recursion level */
    long      keeptmp;     /* Leave temporary files for scan (0=Don't keep, 1=Keep dir) */
    long      load_db_on_startup;/* Load clamav database method (0=Load DB on each scan, 1=Load DB on PHP startup) */
    char     *tmpdir;      /* Temporary directory used on scan */

    struct cl_engine *dbengine;  /* internal db engine */
    unsigned int sig_num;        /* signature number */
    struct cl_stat dbstat;       /* database stat */
    int cl_initcalled;           /* flag for check if cl_init are already called */
ZEND_END_MODULE_GLOBALS(clamav)

/* In every utility function you add that needs to use variables 
   in php_clamav_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as CLAMAV_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define CLAMAV_G(v) TSRMG(clamav_globals_id, zend_clamav_globals *, v)
#else
#define CLAMAV_G(v) (clamav_globals.v)
#endif

#endif    /* PHP_CLAMAV_H */

