#-------------------------------------------------------------------------------
# db
#-------------------------------------------------------------------------------

fips_begin_lib(db)
fips_ide_group(addons)
fips_dir(.)
    fips_files(
        column.cc
        column.h
        command.cc
        command.h
        database.cc
        database.h
        dataset.cc
        dataset.h
        dbfactory.cc
        dbfactory.h
        dbserver.cc
        dbserver.h
        filterset.cc
        filterset.h
        reader.cc
        reader.h
        relation.cc
        relation.h
        table.cc
        table.h
        valuetable.cc
        valuetable.h
        writer.cc
        writer.h
    )
fips_dir(sqlite3)
    fips_files(
        sqlite3command.cc
        sqlite3command.h
        sqlite3database.cc
        sqlite3database.h
        sqlite3dataset.cc
        sqlite3dataset.h
        sqlite3factory.cc
        sqlite3factory.h
        sqlite3filterset.cc
        sqlite3filterset.h
        sqlite3table.cc
        sqlite3table.h
    )
fips_deps(attr sqlite3)
fips_end_lib()

target_precompile_headers(db PRIVATE [["foundation/stdneb.h"]] [["core/refcounted.h"]] [["attr/attrid.h"]] [["db/valuetable.h"]] [["db/table.h"]])
