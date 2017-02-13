#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

void
R_init_rods(DllInfo *info)
{
/* Register routines,
 * allocate resources. */
     // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

}

void
R_unload_rods(DllInfo *info)
{
/* Release resources. */
}
