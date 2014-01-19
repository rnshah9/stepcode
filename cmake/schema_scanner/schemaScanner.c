
/** \file schemaScanner.c
 * This file, along with part of libexpress, are compiled (at configure time)
 * into a static executable. This executable is a schema scanner that is used
 * by CMake to determine what files exp2cxx will create. Otherwise, we'd need
 * to use a few huge files - there is no other way to tell CMake what the
 * generated files will be called.
 */

#include "expparse.h"
#include "expscan.h"

#include "express/scope.h"
#include "genCxxFilenames.h"
#include <string.h>

int multiple_inheritance = 0;

bool isBuiltin( const Type t ) {
    switch( TYPEget_body( t )->type ) {
        case integer_:
        case real_:
        case string_:
        case binary_:
        case boolean_:
        case number_:
        case logical_:
            return true;
        case aggregate_:
        case bag_:
        case set_:
        case list_:
        case array_:
            return true;
            /* this probably always evaluates to true - ought to check */
/*
            if( TYPEget_body( t )->base ) {
                return isBuiltin( TYPEget_body( t )->base );
            }
            return false;
*/
        default:
            break;
    }
    return false;
}

void printSchemaFilenames( Schema sch ){
    DictionaryEntry de;
    Generic x;
    filenames_t fn;
    /* surround the schema name with colons to make it easier to parse the output */
    printf( ":%s:;", sch->symbol.name );
    DICTdo_init( sch->symbol_table, &de );
    while( 0 != ( x = DICTdo( &de ) ) ) {
        switch( DICT_type ) {
            case OBJ_ENTITY:
                fn = getEntityFilenames( ( Entity )x );
                printf( "%s;%s;", fn.impl, fn.header );
                break;
            case OBJ_TYPE: {
                Type t = ( Type ) x;
                if( TYPEis_enumeration( t ) && ( TYPEget_head( t ) ) ) {
                    /* t is a renamed enum type, for which exp2cxx
                     * will print a typedef in an existing file */
                    break;
                }
                if( isBuiltin( t ) ) {
                    /* skip builtin types */
                    break;
                }
                fn = getTypeFilenames( t );
                printf( "%s;%s;", fn.impl, fn.header );
                break;
            }
            /* case OBJ_FUNCTION:
            case OBJ_PROCEDURE:
            case OBJ_RULE: */
            default:
                /* ignore everything else */
                /* TODO: if DEBUG is defined, print the names of these to stderr */
                break;
        }
    }
    printf( "\n" );
}

int main( int argc, char ** argv ) {
    /* TODO init globals! */

    Schema schema;
    DictionaryEntry de;
    /* copied from fedex.c */
    Express model;
    if( ( argc != 2 ) || ( strlen( argv[1] ) < 1 ) ) {
        fprintf( stderr, "\nUsage: %s file.exp\nOutput: one line per schema as follows,", argv[0] );
        fprintf( stderr, " containing file names for entities and types\n" );
        fprintf( stderr, ":schema_name:;entity/e_name.h;entity/e_name.cc;type/t_name.h;type/t_name.cc;...;\n" );
        exit( EXIT_FAILURE );
    }
    EXPRESSprogram_name = argv[0];
    input_filename = argv[1];

    EXPRESSinitialize();

    model = EXPRESScreate();
    EXPRESSparse( model, ( FILE * )0, input_filename );
    if( ERRORoccurred ) {
        EXPRESSdestroy( model );
        exit( EXIT_FAILURE );
    }
    EXPRESSresolve( model );
    if( ERRORoccurred ) {
        int result = EXPRESS_fail( model );
        EXPRESScleanup();
        EXPRESSdestroy( model );
        exit( result );
    }

    DICTdo_type_init( model->symbol_table, &de, OBJ_SCHEMA );
    while( 0 != ( schema = ( Schema )DICTdo( &de ) ) ) {
        printSchemaFilenames( schema );
    }

    EXPRESSdestroy( model );
    exit( EXIT_SUCCESS );
}
