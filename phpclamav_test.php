<?php
/*
 *  Copyright (C) 2011 Argos <argos66@gmail.com>
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

$file = "/var/www/USB.zip";

/* Get running environnement (CLI or not) */
$br = (php_sapi_name() == "cli")? "\n":"<br />\n";

/* Check if extension is loaded and return all functions */
if(!extension_loaded('clamav')) {
	dl('clamav.' . PHP_SHLIB_SUFFIX);
}
$module = 'clamav';
$functions = get_extension_funcs($module);
echo "Functions available in the test extension :".$br;
foreach($functions as $func) {
    echo $func.$br;
}
echo $br.$br;

if (extension_loaded($module)) {

	/* Active cl_debug() for testing, you can found result in your /var/log/apache2/error.log */
    cl_debug();

	/* Run cl_info() and return result */
    echo "<b>cl_info() return : </b>".cl_info().$br;

	/* Run cl_info() and return result */
    echo "<b>cl_version() return : </b>".cl_version().$br;

	/* Run cl_retcode() and return result for a CL_CLEAN and CL_VIRUS */
    echo "<b>cl_pretcode(CL_CLEAN) return : </b>".cl_pretcode(CL_CLEAN).$br;
    echo "<b>cl_pretcode(CL_VIRUS) return : </b>".cl_pretcode(CL_VIRUS).$br;

    /* For future use */
    //echo "<b>cl_engine_get_num(\"CL_ENGINE_MAX_SCANSIZE\") return : </b>".cl_engine_get_num("CL_ENGINE_MAX_SCANSIZE").$br;
    //echo "<b>cl_engine_get_num(\"CL_ENGINE_MAX_FILESIZE\") return : </b>".cl_engine_get_num("CL_ENGINE_MAX_FILESIZE").$br;
    //echo "<b>cl_engine_get_num(\"CL_ENGINE_MAX_RECURSION\") return : </b>".cl_engine_get_num("CL_ENGINE_MAX_RECURSION").$br;

    /* Start counter for bench execution time */
    $time_start = microtime(true);

    /* Run cl_engine() for set the limits values on scan */
    cl_engine(10000, 734003200, 734003200, 25, 0);

    /* Set max_execution_time to 120 second
     *      Only if safe_mode is disabled */
    ini_set('max_execution_time', 120);

	/* Run a cl_scanfile() and return the result into $retcode and the virus name if found in $virusname */
    $retcode = cl_scanfile($file, $virusname);

    /* Restore max_execution_time value from php.ini */
    ini_restore('max_execution_time');

    /* Stop counter for bench execution time */
    $time_end = microtime(true);
    $time = $time_end - $time_start;

    /* Check if a virus founded by scan */
    if ($retcode == CL_VIRUS) {
        echo "<b>Execution time : </b>".round($time, 2)." seconds".$br.
            "<b>File path : </b>".$file.$br.
            "<b>Return code : </b>".cl_pretcode($retcode).$br.
            "<b>Virus found name : </b>".$virusname.$br; 
    } else {
        echo "<b>Execution time : </b>".$time." seconds".$br.
            "<b>File path : </b>".$file.$br.
            "<b>Return code : </b>".cl_pretcode($retcode).$br; 
    }

} else {
	echo "Module $module is not loaded into PHP";
}
?>
